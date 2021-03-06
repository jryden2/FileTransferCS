#pragma once

#include <memory>
#include <fstream>

#include "ILogger.h"
#include "IWorkerThreadPool.h"
#include "ISenderReceiver.h"
#include "IWriter.h"

#include "TransactionManager.h"

class DataTransferServer
{
public:
   DataTransferServer(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool, std::shared_ptr<ISenderReceiver> receiver, std::shared_ptr<IWriterFactory> writerFactory);
   ~DataTransferServer() = default;
   DataTransferServer(const DataTransferServer&) = delete;
   DataTransferServer(DataTransferServer&&) = default;

   void Run();
   void Write(uint32_t transactionID);

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   std::shared_ptr<ISenderReceiver> _senderReceiver;
   std::shared_ptr<IWriterFactory> _writerFactory;
   TransactionManager _manager;
   std::map<uint16_t, std::shared_ptr<IWriter>> _writers;
};

