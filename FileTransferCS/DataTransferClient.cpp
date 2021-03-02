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
      ss << "Client got " << pTransactionUnit->GetMessageLength() << " bytes";
      _logger->Log(0, ss.str());

      // Ensure we got the right cookie, otherwise just drop the message on the floor
      if (pTransactionUnit->IsValid())
      {
         // Okay, this look like a valid message.  See what to do with it, check the message type
         switch (pTransactionUnit->GetMessageType())
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
            ss << "Unknown message type " << pTransactionUnit->GetMessageType();
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

      // Start sequencing at zero
      int sequenceNumber = 0;

      // Create a transaction unit for the start block
      auto pTu = std::make_shared<TransactionUnit>();
      
      pTu->SetMessageData(_reader->GetSource());
      pTu->SetMessageType(MsgType_StartTransaction);
      pTu->SetTransactionID((uint32_t)transactionID);
      pTu->SetSequenceNumber(sequenceNumber++);

      _senderReceiver->Send(pTu);

      while (1)
      {
         // Create a transaction unit for this data block
         pTu = std::make_shared<TransactionUnit>();

         // Read the first block from the file
         auto& messageData = pTu->GetMessageDataVector();
         int read = _reader->Read(messageData);
         if (read == 0) break;

         pTu->SetMessageType(MsgType_Data);
         pTu->SetTransactionID((uint32_t)transactionID);
         pTu->SetSequenceNumber(sequenceNumber++);
         _manager.Add(pTu);

         // Send this block to the server
         _senderReceiver->Send(pTu);
      }

      // Create a transaction unit for the end block
      pTu = std::make_shared<TransactionUnit>();
      pTu->SetMessageData(_reader->GetSource());
     
      pTu->SetMessageType(MsgType_EndTransaction);
      pTu->SetTransactionID((uint32_t)transactionID);
      pTu->SetSequenceNumber(sequenceNumber++);

      _senderReceiver->Send(pTu);
   }
   catch (std::exception& e)
   {
      _logger->Log(5, e.what());
   }
}