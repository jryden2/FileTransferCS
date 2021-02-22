#pragma once

#include "ISenderReceiver.h"
#include "ILogger.h"
#include "IWorkerThreadPool.h"

#include <memory>
#include <functional>
#include <vector>
#include <mutex>

#include <WinSock2.h>
#include <WS2tcpip.h>

class UDPUnreliableSenderReceiver : public ISenderReceiver
{
public:
   UDPUnreliableSenderReceiver(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool);
   ~UDPUnreliableSenderReceiver();
   UDPUnreliableSenderReceiver(const UDPUnreliableSenderReceiver&) = delete;
   UDPUnreliableSenderReceiver(UDPUnreliableSenderReceiver&&) = default;

   void Send(const std::vector<char>& s) override;
   void Receive(std::function<void(std::vector<char>)> callback) override;
   void Start(uint16_t port) override;

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   SOCKET _udpSocket;
   std::function<void(std::vector<char>)> _callback;
   std::mutex _terminateGuard;
};