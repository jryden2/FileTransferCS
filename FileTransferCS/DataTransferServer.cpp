#include "DataTransferServer.h"

#include <sstream>
#include <mutex>

DataTransferServer::DataTransferServer(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool, std::shared_ptr<ISenderReceiver> senderReceiver, std::shared_ptr<IWriterFactory> writerFactory)
      : _logger(logger),
      _threadPool(threadPool),
      _senderReceiver(senderReceiver),
      _writerFactory(writerFactory)
{
   Run();
}

void DataTransferServer::Run()
{
   _senderReceiver->Receive([&](auto pTransactionUnit)
   {
      _threadPool->Post([&, pTransactionUnit]()
      {
         std::lock_guard<std::mutex> lock(_mutex);

         std::stringstream ss;
         ss << "Receiver got " << pTransactionUnit->messagedata.size() << " bytes";
         _logger->Log(0, ss.str());

         // Ensure we got the right cookie, otherwise just drop the message on the floor
         if (pTransactionUnit->IsValid())
         {
            // Okay, this look like a valid message.  See what to do with it, check the message type
            switch (pTransactionUnit->messagetype)
            {
            case MsgType_StartTransaction:
            {
               // This is a new transaction.  Record it and create a writer to represent it.
               auto writer = _writerFactory->Create(_logger);
               _writers[pTransactionUnit->transactionid] = writer;
               std::string s(pTransactionUnit->messagedata.begin(), pTransactionUnit->messagedata.end());

               writer->SetDestination(s);
            }
            break;

            case MsgType_EndTransaction:
            {
               Write(pTransactionUnit->transactionid);
               _writers.erase(pTransactionUnit->transactionid);

               pTransactionUnit->messagelength = 0;
               pTransactionUnit->messagetype = MsgType_RetransmitReq;
               pTransactionUnit->transactionid = (uint32_t)pTransactionUnit->transactionid;
               pTransactionUnit->sequencenum = 0;

               _senderReceiver->Send(pTransactionUnit);
            }
            break;

            case MsgType_Data:
            {
               _manager.Add(pTransactionUnit);

               Write(pTransactionUnit->transactionid);
            }
            break;

            default:
            {
               std::stringstream ss;
               ss << "Unknown message type " << pTransactionUnit->messagetype;
               _logger->Log(3, ss.str());
            }
            break;
            }
         }
      });
   });
}

void DataTransferServer::Write(uint32_t transactionID)
{
   while (1)
   {
      auto pTu = _manager.Collect(transactionID);
      if (pTu)
      {
         std::string s(pTu->messagedata.begin(), pTu->messagedata.end());
         _writers[transactionID]->Write(s);
      }
      else break;
   }
}
