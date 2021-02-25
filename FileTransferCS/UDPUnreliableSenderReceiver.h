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

   void Send(const std::shared_ptr<TransactionUnit>& tu) override;
   void Receive(std::function<void(std::shared_ptr<TransactionUnit>)> callback) override;
   
   void Start(const std::string& remoteIP, uint16_t remotePort, uint16_t localPort);
   void Start(const std::string& ip, uint16_t port);

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   SOCKET _udpSocket;
   std::function<void(std::shared_ptr<TransactionUnit>)> _callback;
   std::mutex _terminateGuard;

   std::string _remoteIP;
   uint16_t _remotePort;

};