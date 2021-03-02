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
         ss << "Receiver got " << pTransactionUnit->GetMessageLength() << " bytes";
         _logger->Log(0, ss.str());

         // Ensure we got the right cookie, otherwise just drop the message on the floor
         if (pTransactionUnit->IsValid())
         {
            // Okay, this look like a valid message.  See what to do with it, check the message type
            switch (pTransactionUnit->GetMessageType())
            {
            case MsgType_StartTransaction:
            case MsgType_EndTransaction:
            case MsgType_Data:
               _manager.Add(pTransactionUnit);
               Write(pTransactionUnit);
            break;

            default:
            {
               std::stringstream ss;
               ss << "Unknown message type " << pTransactionUnit->GetMessageType();
               _logger->Log(3, ss.str());
            }
            break;
            }
         }
      });
   });
}

void DataTransferServer::Write(std::shared_ptr<TransactionUnit> pTu)
{
   while (1)
   {
      pTu = _manager.Collect(pTu->GetTransactionID());
      if (pTu)
      {
         switch (pTu->GetMessageType())
         {
         case MsgType_Data:
         {
            std::string s;
            pTu->GetMessageData(s);
            auto iter = _writers.find(pTu->GetTransactionID());
            if (iter != _writers.end())
            {
               iter->second->Write(s);
            }
            else
            {
               // No writer set up yet, maybe we're waiting for the start message?  Put it back in the buffer
               _manager.Add(pTu);
            }
         }
         break;

         case MsgType_RetransmitReq:
            // Missing packet indication, send it back to the client side...
            _senderReceiver->Send(pTu);
            break;

         case MsgType_StartTransaction:
         {
            // This is a new transaction.  Record it and create a writer to represent it.
            auto writer = _writerFactory->Create(_logger);
            _writers[pTu->GetTransactionID()] = writer;

            std::string s;
            pTu->GetMessageData(s);

            writer->SetDestination(s);
         }
         break;

         case MsgType_EndTransaction:
            _writers.erase(pTu->GetTransactionID());
            break;

         default:
            // Server handles no other messages, just drop them
            break;
         };
      }
      else break;
   }
}
