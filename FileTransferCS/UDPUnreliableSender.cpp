#include "UDPUnreliableSender.h"

#include <sstream>

UDPUnreliableSender::UDPUnreliableSender(std::shared_ptr<ILogger> logger)
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

UDPUnreliableSender::~UDPUnreliableSender()
   {
      closesocket(_udpSocket);
      WSACleanup();
   }

void UDPUnreliableSender::Send(const std::vector<char>& s)
{
   sockaddr_in addr;

   addr.sin_family = AF_INET;
   addr.sin_port = htons(1234);
   inet_pton(AF_INET, "127.0.0.1", (void*)&addr.sin_addr.s_addr);

   sendto(_udpSocket, s.data(), s.size(), 0, (sockaddr*)&addr, sizeof(addr));
}

