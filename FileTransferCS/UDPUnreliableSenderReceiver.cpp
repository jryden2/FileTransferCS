#include "UDPUnreliableSenderReceiver.h"

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

void UDPUnreliableSenderReceiver::Start(uint16_t port)
{
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
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

UDPUnreliableSenderReceiver::~UDPUnreliableSenderReceiver()
{
   closesocket(_udpSocket);

   // This is a bit of kludge.  A message posted to a 'stranded' thread execution would be best, but strands are not yet implemented in the thread pool
   std::lock_guard<std::mutex> lock(_terminateGuard);

   WSACleanup();
}

void UDPUnreliableSenderReceiver::Send(const std::vector<char>& s)
{
   sockaddr_in addr;

   addr.sin_family = AF_INET;
   addr.sin_port = htons(1234);
   inet_pton(AF_INET, "127.0.0.1", (void*)&addr.sin_addr.s_addr);

   std::stringstream ss;
   ss << "Sending " << s.size() << " bytes";
   _logger->Log(0, ss.str());

   sendto(_udpSocket, s.data(), s.size(), 0, (sockaddr*)&addr, sizeof(addr));
}

void UDPUnreliableSenderReceiver::Receive(std::function<void(std::vector<char>)> callback)
{
   _callback = callback;
}