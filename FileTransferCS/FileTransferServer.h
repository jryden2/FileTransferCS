#pragma once

#include <memory>
#include <fstream>

#include "ILogger.h"
#include "IWorkerThreadPool.h"
#include "IReceiver.h"
#include "IWriter.h"

#include "TransactionManager.h"

class FileTransferServer
{
public:
   FileTransferServer(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool, std::shared_ptr<IReceiver> receiver, std::shared_ptr<IWriterFactory> writerFactory);
   ~FileTransferServer() = default;
   FileTransferServer(const FileTransferServer&) = delete;
   FileTransferServer(FileTransferServer&&) = default;

   void Run();
   void Write(uint32_t transactionID);

private:
   std::shared_ptr<ILogger> _logger;
   std::shared_ptr<IWorkerThreadPool> _threadPool;
   std::shared_ptr<IReceiver> _receiver;
   std::shared_ptr<IWriterFactory> _writerFactory;
   TransactionManager _manager;
   std::map<uint16_t, std::shared_ptr<IWriter>> _writers;
};

