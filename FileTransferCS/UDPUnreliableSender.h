#pragma once

#include "ISender.h"
#include "ILogger.h"

#include <memory>
#include <vector>

#include <WinSock2.h>
#include <WS2tcpip.h>

class UDPUnreliableSender : public ISender
{
public:
   UDPUnreliableSender(std::shared_ptr<ILogger> logger);
   ~UDPUnreliableSender();
   UDPUnreliableSender(const UDPUnreliableSender&) = delete;
   UDPUnreliableSender(UDPUnreliableSender&&) = default;

   void Send(const std::vector<char>& s) override;
   
private:
   std::shared_ptr<ILogger> _logger;

   SOCKET _udpSocket;
};

