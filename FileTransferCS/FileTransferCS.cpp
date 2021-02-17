// FileTransferCS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 
// Winsock2 - Basic setup for UDP sockets pulled from various examples in https://docs.microsoft.com/en-us/windows/win32/api/winsock/
//

#include <fstream>
#include <iostream>
#include <functional>
#include <vector>
#include <map>

#include <winsock2.h>
#include <WS2tcpip.h>

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
      std::cout << "[Sev:" << level << "] " << s;
   }
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
   virtual void Read(std::vector<char>& s) = 0;
};

const static int _blockSize = 128;

class FileReader : public IReader
{
public:
   FileReader(std::shared_ptr<ILogger> logger)
      : _logger(logger)
   {}
   
   ~FileReader() = default;

   void Read(std::vector<char>& s) override
   {
      // Open and read file data
      std::fstream f(_filename);
      f.read(s.data(), s.size());
   };

   void SetFile(const std::string& filename) { _filename = filename; }

private:
   std::shared_ptr<ILogger> _logger;
   std::string _filename;
};


class ISender
{
public:
   virtual void Send(const std::vector<char>& s) = 0;
};

class UDPSender : public ISender
{
public:
   UDPSender(std::shared_ptr<ILogger> logger)
      : _logger(logger)
   {
      WSADATA wsa;
      if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
      {
         throw std::runtime_error("WSAStartup failed");
      }

      _udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (_udpSocket == INVALID_SOCKET) throw std::runtime_error("create socket failed");

      _addr.sin_family = AF_INET;
      _addr.sin_port = htons(1234);
      IN_ADDR inaddr;
      _addr.sin_addr.s_addr = inet_pton(AF_INET, "127.0.0.1", (void*)&inaddr);
   }

   ~UDPSender()
   {
      closesocket(_udpSocket);
      WSACleanup();
   }

   void Send(const std::vector<char>& s) override
   {

   }

private:
   std::shared_ptr<ILogger> _logger;

   SOCKET _udpSocket;
   sockaddr_in _addr;
};

#include <winsock2.h>
class IReceiver
{
public:
   //virtual void Receive(const std::vector<char>& s) = 0;
};

class UDPReceiver : public IReceiver
{
public:
   UDPReceiver(std::shared_ptr<ILogger> logger)
      : _logger(logger)
   {
      WSADATA wsa;
      if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
      {
         throw std::runtime_error("WSAStartup failed");
      }

      _udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
      if (_udpSocket == INVALID_SOCKET) throw std::runtime_error("create socket failed");

      _addr.sin_family = AF_INET;
      _addr.sin_port = htons(1234);
      _addr.sin_addr.s_addr = INADDR_ANY;


   }

   ~UDPReceiver()
   {
      closesocket(_udpSocket);
      WSACleanup();
   }

private:
   std::shared_ptr<ILogger> _logger;

   SOCKET _udpSocket;
   sockaddr_in _addr;
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
   uint32_t cookie;
   uint32_t transactionid;
   uint16_t messagetype;
   uint16_t messagelength;
   uint16_t sequencenum;
   uint16_t reserved;
   std::vector<char> messagedata;
};

class TransactionManager
{
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

class FileTransferCS
{
public:
   FileTransferCS()
   {
      _logger = std::make_shared<SimpleLogger>();

      // Create a UDP Server to collect data
      _server = std::make_shared<UDPReceiver>(_logger);
   }

   void Run()
   {
      try
      {
         // Create a file reader for the requested file
         auto reader = std::make_unique<FileReader>(_logger);
         reader->SetFile("");

         // Read the first block from the file
         std::vector<char> s;
         s.resize(_blockSize);
         reader->Read(s);

         // Send this block to the server
         auto sender = std::make_unique<UDPSender>(_logger);
         sender->Send(s);
      }
      catch (std::exception& e)
      {
         _logger->Log(5, e.what());
      }
   }

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IReceiver> _server;
};



int main()
{
   FileTransferCS ft;
   ft.Run();
}
