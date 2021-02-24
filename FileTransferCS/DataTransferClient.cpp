#include "DataTransferClient.h"

#include <random>
#include <sstream>

DataTransferClient::DataTransferClient(std::shared_ptr<ILogger> logger, 
                                       std::shared_ptr<IWorkerThreadPool> threadPool, 
                                       std::shared_ptr<IReader> reader, 
                                       std::shared_ptr<ISenderReceiver> senderReceiver)
   : _logger(logger),
   _threadPool(threadPool),
   _reader(reader),
   _senderReceiver(senderReceiver)
{
   RunReceiver();
   RunSender();
}

void DataTransferClient::RunReceiver()
{
   _senderReceiver->Receive([&](auto pTransactionUnit)
   {
      std::stringstream ss;
      ss << "Client got " << pTransactionUnit->messagedata.size() << " bytes";
      _logger->Log(0, ss.str());

      // Ensure we got the right cookie, otherwise just drop the message on the floor
      if (pTransactionUnit->IsValid())
      {
         // Okay, this look like a valid message.  See what to do with it, check the message type
         switch (pTransactionUnit->messagetype)
         {
         case MsgType_RetransmitReq:
         {
            std::stringstream ss;
            ss << "Client got retransmit request";
            _logger->Log(0, ss.str());
         }
         break;

         case MsgType_StartTransaction:
         case MsgType_EndTransaction:
         case MsgType_Data:
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
}

void DataTransferClient::RunSender()
{
   try
   {
      // Create a random number generator for the transaction id
      std::random_device rd;
      std::mt19937 mt(rd());
      std::uniform_real_distribution<double> dist(0, 0xFFFF);
      auto transactionID = dist(mt);

      // Start sequencing a zero
      int sequenceNumber = 0;

      // Create a transaction unit for the start block
      auto pTU = std::make_shared<TransactionUnit>();
      
      auto source = _reader->GetSource();
      pTU->messagedata.assign(source.begin(), source.end());
      pTU->messagelength = (uint16_t)source.size();
      pTU->messagetype = MsgType_StartTransaction;
      pTU->transactionid = (uint32_t)transactionID;
      pTU->sequencenum = 0;

      _senderReceiver->Send(pTU);

      while (1)
      {
         // Create a transaction unit for this block
         auto pTU = std::make_shared<TransactionUnit>();

         // Read the first block from the file
         int read = _reader->Read(pTU->messagedata);
         if (read == 0) break;

         pTU->messagetype = MsgType_Data;
         pTU->messagelength = (uint16_t)pTU->messagedata.size();
         pTU->transactionid = (uint32_t)transactionID;
         pTU->sequencenum = sequenceNumber++;
         _manager.Add(pTU);

         // Send this block to the server
         _senderReceiver->Send(pTU);
      }

      // Create a transaction unit for the end block
      pTU = std::make_shared<TransactionUnit>();
      pTU->messagedata.assign(source.begin(), source.end());
      pTU->messagelength = (uint16_t)source.size();
      pTU->messagetype = MsgType_EndTransaction;
      pTU->transactionid = (uint32_t)transactionID;
      pTU->sequencenum = 0;

      _senderReceiver->Send(pTU);
   }
   catch (std::exception& e)
   {
      _logger->Log(5, e.what());
   }
}