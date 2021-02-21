#pragma once

#include <memory>

#include "ILogger.h"
#include "IWorkerThreadPool.h"
#include "IReader.h"
#include "ISender.h"

#include "TransactionManager.h"

class DataTransferClient
{
public:
   DataTransferClient(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool, std::shared_ptr<IReader> reader, std::shared_ptr<ISender> sender);

   void Run();

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   std::shared_ptr<IReader> _reader;
   std::shared_ptr<ISender> _sender;

   TransactionManager _manager;
};
