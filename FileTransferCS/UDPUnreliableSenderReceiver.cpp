#include "UDPUnreliableSenderReceiver.h"
#include "TransactionUnit.h"

#include <sstream>

UDPUnreliableSenderReceiver::UDPUnreliableSenderReceiver(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool)
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
}

void UDPUnreliableSenderReceiver::Start(const std::string& remoteIP, uint16_t remotePort)
{
   // Local port 0, binds to an ephemeral port
   Start(remoteIP, remotePort, 0);
}

void UDPUnreliableSenderReceiver::Start(const std::string& remoteIP, uint16_t remotePort, uint16_t localPort)
{
   _remoteIP = remoteIP;
   _remotePort = remotePort;

   // Bind to the requested local port
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(localPort);
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
      std::lock_guard<std::mutex> lock(_terminateGuard);

      while (1)
      {
         sockaddr_in fromAddr;
         int fromlen = sizeof(fromAddr);
         std::vector<char> buffer;
         buffer.resize(0x1000);
         int bytes = recvfrom(_udpSocket, buffer.data(), buffer.size(), 0, (sockaddr*)&fromAddr, &fromlen);

         if (bytes == SOCKET_ERROR)
         {
            std::stringstream ss;
            ss << "recvfrom failed, rc=" << WSAGetLastError();
            _logger->Log(5, ss.str());
            break;
         }

         buffer.resize(bytes);

         std::stringstream ss;
         char ip[20];
         ss << "Received " << bytes << " bytes from " << inet_ntop(AF_INET, (void*)&fromAddr.sin_addr, (PSTR)&ip, sizeof(ip)) << ":" << ntohs(fromAddr.sin_port);
         _logger->Log(0, ss.str());

         // Handle the new packet
         auto tu = std::make_shared<TransactionUnit>(buffer);
         tu->SetRemoteAddress(fromAddr);

         _callback(tu);
      }
   });

}

UDPUnreliableSenderReceiver::~UDPUnreliableSenderReceiver()
{
   closesocket(_udpSocket);

   // This is a bit of kludge.  A message posted to a 'stranded' thread execution would be best, but strands are not yet implemented in the thread pool
   std::lock_guard<std::mutex> lock(_terminateGuard);

   WSACleanup();
}

void UDPUnreliableSenderReceiver::Send(const std::shared_ptr<TransactionUnit>& pTu)
{
   std::vector<char> buffer;
   pTu->GetBlob(buffer);

   // if pTu has an ip and port set, use it, otherwise, use the stored data to generate the addr
   auto addr = pTu->GetRemoteAddress();
   if (addr.sin_port == 0)
   {
      addr.sin_family = AF_INET;
      addr.sin_port = htons(_remotePort);
      inet_pton(AF_INET, _remoteIP.c_str(), (void*)&addr.sin_addr.s_addr);
   }

   std::stringstream ss;
   char ip[20];
   ss << "Sending " << buffer.size() << " bytes to " << inet_ntop(AF_INET, (void*)&addr.sin_addr, (PSTR)&ip, sizeof(ip)) << ":" << ntohs(addr.sin_port);
   _logger->Log(0, ss.str());

   sendto(_udpSocket, buffer.data(), buffer.size(), 0, (sockaddr*)&addr, sizeof(addr));
}

void UDPUnreliableSenderReceiver::Receive(std::function<void(std::shared_ptr<TransactionUnit>)> callback)
{
   _callback = callback;
}