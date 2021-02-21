#include "FileTransferServer.h"

#include <sstream>

DataTransferServer::DataTransferServer(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool, std::shared_ptr<IReceiver> receiver, std::shared_ptr<IWriterFactory> writerFactory)
      : _logger(logger),
      _threadPool(threadPool),
      _receiver(receiver),
      _writerFactory(writerFactory)
{
   Run();
}

void DataTransferServer::Run()
{
   _receiver->Receive([&](auto buf)
   {
      // Handle the new packet
      auto tu = std::make_shared<TransactionUnit>(buf);

      std::stringstream ss;
      ss << "Receiver got " << buf.size() << " bytes";
      _logger->Log(0, ss.str());

      // Ensure we got the right cookie, otherwise just drop the message on the floor
      if (tu->IsValid())
      {
         // Okay, this look like a valid message.  See what to do with it, check the message type
         switch (tu->messagetype)
         {
         case MsgType_StartTransaction:
         {
            // This is a new transaction.  Record it and create a writer to represent it.
            auto writer = _writerFactory->Create();
            _writers[tu->transactionid] = writer;
            std::string s(tu->messagedata.begin(), tu->messagedata.end());

            std::stringstream ss;
            ss << ".\\Received\\" << s;
            writer->SetDestination(ss.str());
         }
         break;

         case MsgType_EndTransaction:
         {
            Write(tu->transactionid);
            _writers.erase(tu->transactionid);
         }
         break;

         case MsgType_Data:
         {
            _manager.Add(tu);

            Write(tu->transactionid);
         }
         break;

         default:
         {
            std::stringstream ss;
            ss << "Unknown message type " << tu->messagetype;
            _logger->Log(3, ss.str());
         }
         break;
         }
      }
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
