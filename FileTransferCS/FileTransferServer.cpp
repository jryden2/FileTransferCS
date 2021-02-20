#include "FileTransferServer.h"

#include <sstream>

FileTransferServer::FileTransferServer(std::shared_ptr<ILogger> logger, std::shared_ptr<IWorkerThreadPool> threadPool, std::shared_ptr<IReceiver> receiver)
      : _logger(logger),
      _threadPool(threadPool),
      _receiver(receiver)
{
   Run();
}

void FileTransferServer::Run()
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
            // This is a new transaction.  Record it and create the file it represents.
            auto& f = _files[tu->transactionid];
            std::string s(tu->messagedata.begin(), tu->messagedata.end());
            std::stringstream ss;
            ss << ".\\Received\\" << s;
            f.open(ss.str());
         }
         break;

         case MsgType_EndTransaction:
         {
            Write(tu->transactionid);

            auto& f = _files[tu->transactionid];
            f.close();
            _files.erase(tu->transactionid);
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

void FileTransferServer::Write(uint32_t transactionID)
{
   while (1)
   {
      auto pTu = _manager.Collect(transactionID);
      if (pTu)
      {
         std::string s(pTu->messagedata.begin(), pTu->messagedata.end());
         _files[transactionID] << s;
      }
      else break;
   }
}
