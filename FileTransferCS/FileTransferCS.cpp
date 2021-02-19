// FileTransferCS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 
// Winsock2 - Basic setup for UDP sockets pulled from various examples in https://docs.microsoft.com/en-us/windows/win32/api/winsock/
//

#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <random>

#include <winsock2.h>
#include <WS2tcpip.h>

#include "WorkerThreadPool.h"

class ILogger
{
public:
   virtual void Log(int level, const std::string& s) = 0;
};

class SimpleLogger : public ILogger
{
public:
   void Log(int level, const std::string& s) override
   {
      std::stringstream ss;
      ss << "[Sev:" << level << "] " << s << std::endl;
      std::cout << ss.str();

#ifdef _WIN32
      OutputDebugString(ss.str().c_str());
#endif
   }
};




// Transmission headers
//
//        0                   1                   2                   3
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |          32 bit cookie (minor security and versioning)        |
//       |                 32 bit transaction id                         |
//       |       Msg type                  |        Length               |
//       |       16 bit Sequence #         |        Reserved             |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                                                               |
//       |                         Message data                          |
//       |                                                               |
//       .                                                               .
//       .                                                               .
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

static const int MagicCookie = 0xA343F33B;               // A magic cookie to recognize our activity
// Message types
enum MsgType
{
   MsgType_StartTransaction = 0x0001,  // Message data contains filename (Sequence #0 expected)
   MsgType_EndTransaction = 0x0002,    // Message data empty
   MsgType_RetransmitReq = 0x0003,     // Message data empty (Sequence number is requested sequence)
   MsgType_Data = 0x0004,              // Message data contains a message blob of specified size
};

struct TransmissionUnit
{
   TransmissionUnit(const std::vector<char> buffer)
      : _mySize(16)
   {
      auto buf = buffer.data();

      memcpy(&cookie, buf, sizeof(cookie));
      buf += sizeof(cookie);

      memcpy(&transactionid, buf, sizeof(transactionid));
      buf += sizeof(transactionid);

      memcpy(&messagetype, buf, sizeof(messagetype));
      buf += sizeof(messagetype);

      memcpy(&messagelength, buf, sizeof(messagelength));
      buf += sizeof(messagelength);

      memcpy(&sequencenum, buf, sizeof(sequencenum));
      buf += sizeof(sequencenum);

      memcpy(&reserved, buf, sizeof(reserved));
      buf += sizeof(reserved);

      messagedata.resize(messagelength);
      messagedata.assign(buf, buf + messagelength);

      _isValid = cookie == MagicCookie;
   }

   TransmissionUnit()
      : _mySize(16),
        _isValid(true),
        cookie(MagicCookie),
        reserved(0)
   {}

   bool IsValid() { return _isValid; }

   void GetBlob(std::vector<char>& buffer)
   {
      buffer.resize(_mySize + messagedata.size());

      char* buf = buffer.data();

      memcpy(buf, &cookie, sizeof(cookie));
      buf += sizeof(cookie);

      memcpy(buf, &transactionid, sizeof(transactionid));
      buf += sizeof(transactionid);

      memcpy(buf, &messagetype, sizeof(messagetype));
      buf += sizeof(messagetype);

      memcpy(buf, &messagelength, sizeof(messagelength));
      buf += sizeof(messagelength);

      memcpy(buf, &sequencenum, sizeof(sequencenum));
      buf += sizeof(sequencenum);

      memcpy(buf, &sequencenum, sizeof(sequencenum));
      buf += sizeof(sequencenum);

      memcpy(buf, messagedata.data(), messagedata.size());
   }

   uint32_t cookie;
   uint32_t transactionid;
   uint16_t messagetype;
   uint16_t messagelength;
   uint16_t sequencenum;
   uint16_t reserved;

   std::vector<char> messagedata;

private:
   const uint32_t _mySize;
   bool _isValid;
};

class TransactionManager
{
public:
   TransactionManager() = default;
   ~TransactionManager() = default;

   void Add(std::shared_ptr<TransmissionUnit> tu)
   {
      // Find the associated transaction and add the unit to the list of pending messages
      auto iter = unconfirmedPackets.find(tu->transactionid);
      if (iter != unconfirmedPackets.end())
      {
         iter->second.insert(std::make_pair(tu->sequencenum, tu));
      }
      else
      {
         // Create a new transaction for this unit
         unconfirmedPackets[tu->transactionid].insert(std::make_pair(tu->sequencenum, tu));
      }
   }

private:
   std::map<uint32_t, std::map<uint16_t, std::shared_ptr<TransmissionUnit>>> unconfirmedPackets;
};









class INetworkListener
{
public:
   virtual void Listen() = 0;
};

class IStreamCallback
{
   virtual void Process(std::function<void()> callback) = 0;
};

class IDatacache
{
   virtual void Store() = 0;
   virtual void Retrieve() = 0;
};

class IReader
{
public:
   virtual uint32_t Read(std::vector<char>& s) = 0;
};

const static int _blockSize = 128;

class FileReader : public IReader
{
public:
   FileReader(std::shared_ptr<ILogger> logger)
      : _logger(logger)
   {}
   
   ~FileReader() = default;

   uint32_t Read(std::vector<char>& s) override
   {
      // Open and read file data
      _fileStream.read(s.data(), s.size());

      auto count = _fileStream.gcount();
      s.resize(count);
      return count;
   };

   void SetFile(const std::string& filename) 
   {
      _fileStream.open(filename);
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::fstream _fileStream;
};


class ISender
{
public:
   virtual void Send(const std::vector<char>& s) = 0;
};

class UDPUnreliableSender : public ISender
{
public:
   UDPUnreliableSender(std::shared_ptr<ILogger> logger)
      : _logger(logger)
   {
      WSADATA wsa;
      if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
      {
         throw std::runtime_error("WSAStartup failed");
      }

      _udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (_udpSocket == INVALID_SOCKET) throw std::runtime_error("create socket failed");

   }

   ~UDPUnreliableSender()
   {
      closesocket(_udpSocket);
      WSACleanup();
   }

   void Send(const std::vector<char>& s) override
   {
      sockaddr_in addr;

      addr.sin_family = AF_INET;
      addr.sin_port = htons(1234);
      inet_pton(AF_INET, "127.0.0.1", (void*)&addr.sin_addr.s_addr);

      sendto(_udpSocket, s.data(), s.size(), 0, (sockaddr*)&addr, sizeof(addr));
   }

private:
   std::shared_ptr<ILogger> _logger;

   SOCKET _udpSocket;
};

class IReceiver
{
public:
   virtual void Receive(std::function<void(std::vector<char>)> callback) = 0;
};

class UDPReceiver : public IReceiver
{
public:
   UDPReceiver(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool)
      : _logger(logger),
        _threadPool(threadPool)
   {
      WSADATA wsa;
      if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
      {
         throw std::runtime_error("WSAStartup failed");
      }

      _udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (_udpSocket == INVALID_SOCKET) throw std::runtime_error("create socket failed");

      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(1234);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      int result = bind(_udpSocket, (sockaddr*)&addr, sizeof(addr));
      if (result == SOCKET_ERROR)
      {
         std::stringstream ss;
         ss << "bind failed, rc=" << WSAGetLastError();
         _logger->Log(5, ss.str());
      }

      _threadPool->Post([&]
      {
         while (1)
         {
            sockaddr_in from;
            int fromlen = sizeof(from);
            std::vector<char> buf;
            buf.resize(0x1000);
            int bytes = recvfrom(_udpSocket, buf.data(), buf.size(), 0, (sockaddr*)&from, &fromlen);

            if (bytes == SOCKET_ERROR)
            {
               std::stringstream ss;
               ss << "recvfrom failed, rc=" << WSAGetLastError();
               _logger->Log(5, ss.str());
               break;
            }

            buf.resize(bytes);

            std::stringstream ss;
            char ip[20];
            ss << "Received " << bytes << " bytes from " << inet_ntop(AF_INET, (void*)&from.sin_addr, (PSTR)&ip, sizeof(ip)) << ":" << ntohs(from.sin_port);
            _logger->Log(0, ss.str());

            _callback(buf);

         }
      });
   }

   ~UDPReceiver()
   {
      closesocket(_udpSocket);
      WSACleanup();
   }

   void Receive(std::function<void(std::vector<char>)> callback) override
   {
      _callback = callback;
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   SOCKET _udpSocket;
   std::function<void(std::vector<char>)> _callback;
};



class FileTransferCS
{
public:
   FileTransferCS()
   {
      _logger = std::make_shared<SimpleLogger>();
      
      _threadPool = std::make_shared<WorkerThreadPool>();

      // Create a UDP Server to collect data
      _server = std::make_shared<UDPReceiver>(_logger, _threadPool);
               

      _server->Receive([&](auto buf)
      {
         // Handle the new packet
         auto tu = std::make_shared<TransmissionUnit>(buf);

         std::stringstream ss;
         ss << "Receiver got " << buf.size() << " bytes";
         _logger->Log(0, ss.str());

         // Ensure we got the right cookie, otherwise just drop the message on the floor
         if (tu->IsValid())
         {
            // Okay, valid message.  See what to do with it
            switch (tu->messagetype)
            {
            case MsgType_StartTransaction:
            {
               // This is a new transaction.  Record it and create the file it represents.
               auto& f = _files[tu->transactionid];
               std::string s(tu->messagedata.begin(), tu->messagedata.end());
               std::stringstream ss; 
               ss << ".\\Received\\" << s;
               f.open(ss.str());
            }
            break;

            case MsgType_EndTransaction:
            {
               auto& f = _files[tu->transactionid];
               f.close();
               _files.erase(tu->transactionid);
            }
            break;

            case MsgType_Data:
               _files[tu->transactionid] << tu->messagedata.data();
               break;

            default:
            {
               std::stringstream ss;
               ss << "Unknown message type " << tu->messagetype;
               _logger->Log(3, ss.str());
            }
            break;
            }
         }
      });
   }

   void Run()
   {
      try
      {
         // Create the sender device
         auto sender = std::make_unique<UDPUnreliableSender>(_logger);

         std::vector<char> buffer;

         // Create a random number generator for the transaction id
         std::random_device rd;
         std::mt19937 mt(rd());
         std::uniform_real_distribution<double> dist(0, 0xFFFF);
         auto transactionID = dist(mt);

         // Start sequencing a zero
         int sequenceNumber = 0;

         std::string filename = "test.txt";

         // Create a transmission unit for the start block
         auto tu = std::make_shared<TransmissionUnit>();
         tu->messagedata.assign(filename.begin(), filename.end());
         tu->messagelength = filename.size();
         tu->messagetype = MsgType_StartTransaction;
         tu->transactionid = transactionID;
         tu->sequencenum = sequenceNumber++;
         _manager.Add(tu);

         tu->GetBlob(buffer);
         sender->Send(buffer);


         // Create a file reader for the requested file
         auto reader = std::make_unique<FileReader>(_logger);
         reader->SetFile(filename);

         while (1)
         {
            // Create a transmission unit for this block
            auto tu = std::make_shared<TransmissionUnit>();

            // Read the first block from the file
            tu->messagedata.resize(_blockSize);
            int read = reader->Read(tu->messagedata);

            tu->messagetype = MsgType_Data;
            tu->messagelength = tu->messagedata.size();
            tu->transactionid = transactionID;
            tu->sequencenum = sequenceNumber++;
            _manager.Add(tu);

            // Send this block to the server
            tu->GetBlob(buffer);
            sender->Send(buffer);

            if (read < _blockSize) break;

         }

         // Create a transmission unit for the end block
         tu = std::make_shared<TransmissionUnit>();
         tu->messagedata.assign(filename.begin(), filename.end());
         tu->messagelength = filename.size();
         tu->messagetype = MsgType_EndTransaction;
         tu->transactionid = transactionID;
         tu->sequencenum = sequenceNumber++;
         _manager.Add(tu);

         tu->GetBlob(buffer);
         sender->Send(buffer);

         // Todo: Hang out for a while waiting for retransmit requests
         Sleep(10000);
      }
      catch (std::exception& e)
      {
         _logger->Log(5, e.what());
      }
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IReceiver> _server;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   TransactionManager _manager;

   std::map<uint16_t, std::ofstream> _files;
};



int main()
{
   FileTransferCS ft;
   ft.Run();
}
