#pragma once

#include "IReceiver.h"
#include "ILogger.h"
#include "IWorkerThreadPool.h"

#include <memory>
#include <functional>
#include <vector>
#include <mutex>

#include <WinSock2.h>
#include <WS2tcpip.h>

class UDPReceiver : public IReceiver
{
public:
   UDPReceiver(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool);
   ~UDPReceiver();
 
   void Receive(std::function<void(std::vector<char>)> callback) override;

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   SOCKET _udpSocket;
   std::function<void(std::vector<char>)> _callback;
   std::mutex _terminateGuard;

};
