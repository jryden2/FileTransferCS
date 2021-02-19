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
#include <set>
#include <random>

#include <winsock2.h>
#include <WS2tcpip.h>

#include "WorkerThreadPool.h"
#include "SimpleLogger.h"

#include "TransactionUnit.h"
#include "TransactionManager.h"

class IReader
{
public:
   virtual uint32_t Read(std::vector<char>& s) = 0;
};

class FileReader : public IReader
{
public:
   FileReader(std::shared_ptr<ILogger> logger)
      : _logger(logger),
        _blockSize(128)
   {}
   
   ~FileReader() = default;

   uint32_t Read(std::vector<char>& s) override
   {
      // Open and read file data
      s.resize(_blockSize);
      _fileStream.read(s.data(), s.size());

      auto count = _fileStream.gcount();
      s.resize((size_t)count);
      return (uint32_t)count;
   };

   void SetFile(const std::string& filename) 
   {
      _fileStream.open(filename);
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::fstream _fileStream;
   const uint8_t _blockSize;
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

class FileTransferServer
{
public:
   FileTransferServer(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool)
      : _logger(logger),
      _threadPool(threadPool)
   {
      Run();
   }

   void Run()
   {
      _server = std::make_shared<UDPReceiver>(_logger, _threadPool);
      _server->Receive([&](auto buf)
      {
         // Handle the new packet
         auto tu = std::make_shared<TransactionUnit>(buf);

         std::stringstream ss;
         ss << "Receiver got " << buf.size() << " bytes";
         _logger->Log(0, ss.str());

         // Ensure we got the right cookie, otherwise just drop the message on the floor
         if (tu->IsValid())
         {
            // Okay, this look like a valid message.  See what to do with it, check the message type
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
               Write(tu->transactionid);

               auto& f = _files[tu->transactionid];
               f.close();
               _files.erase(tu->transactionid);
            }
            break;

            case MsgType_Data:
            {
               _manager.Add(tu);

               Write(tu->transactionid);
            }
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

   void Write(uint32_t transactionID)
   {
      while (1)
      {
         auto pTu = _manager.Collect(transactionID);
         if (pTu)
         {
            std::string s(pTu->messagedata.begin(), pTu->messagedata.end());
            _files[transactionID] << s;
         }
         else break;
      }
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   std::shared_ptr<IReceiver> _server;
   TransactionManager _manager;
   std::map<uint16_t, std::ofstream> _files;
};

class FileTransferClient
{
public:
   FileTransferClient(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool)
      : _logger(logger),
        _threadPool(threadPool)
   {
      Run();
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
         auto tu = std::make_shared<TransactionUnit>();
         tu->messagedata.assign(filename.begin(), filename.end());
         tu->messagelength = (uint16_t)filename.size();
         tu->messagetype = MsgType_StartTransaction;
         tu->transactionid = (uint32_t)transactionID;
         tu->sequencenum = 0;
         //_manager.Add(tu);

         tu->GetBlob(buffer);
         sender->Send(buffer);


         // Create a file reader for the requested file
         auto reader = std::make_unique<FileReader>(_logger);
         reader->SetFile(filename);

         while (1)
         {
            // Create a transmission unit for this block
            auto tu = std::make_shared<TransactionUnit>();

            // Read the first block from the file
            int read = reader->Read(tu->messagedata);
            if (read == 0) break;

            tu->messagetype = MsgType_Data;
            tu->messagelength = (uint16_t)tu->messagedata.size();
            tu->transactionid = (uint32_t)transactionID;
            tu->sequencenum = sequenceNumber++;
            //_manager.Add(tu);

            // Send this block to the server
            tu->GetBlob(buffer);
            sender->Send(buffer);
         }

         // Create a transmission unit for the end block
         tu = std::make_shared<TransactionUnit>();
         tu->messagedata.assign(filename.begin(), filename.end());
         tu->messagelength = (uint16_t)filename.size();
         tu->messagetype = MsgType_EndTransaction;
         tu->transactionid = (uint32_t)transactionID;
         tu->sequencenum = 0;
         //_manager.Add(tu);

         tu->GetBlob(buffer);
         sender->Send(buffer);

         // Todo: Hang out for a while waiting for retransmit requests
         char q;
         std::cin >> q;
      }
      catch (std::exception& e)
      {
         _logger->Log(5, e.what());
      }
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   TransactionManager _manager;

};



int main()
{
   // Create a UDP Server to collect data
   auto logger = std::make_shared<SimpleLogger>();
   auto threadPool = std::make_shared<WorkerThreadPool>();

   FileTransferServer fts(logger, threadPool);
   FileTransferClient ftc(logger, threadPool);
}
