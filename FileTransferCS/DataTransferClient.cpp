#include "FileTransferClient.h"

#include <random>

DataTransferClient::DataTransferClient(std::shared_ptr<ILogger> logger, 
                                       std::shared_ptr<IWorkerThreadPool> threadPool, 
                                       std::shared_ptr<IReader> reader, 
                                       std::shared_ptr<ISender> sender)
   : _logger(logger),
   _threadPool(threadPool),
   _reader(reader),
   _sender(sender)
{
   Run();
}

void DataTransferClient::Run()
{
   try
   {
      std::vector<char> buffer;

      // Create a random number generator for the transaction id
      std::random_device rd;
      std::mt19937 mt(rd());
      std::uniform_real_distribution<double> dist(0, 0xFFFF);
      auto transactionID = dist(mt);

      // Start sequencing a zero
      int sequenceNumber = 0;

      // Create a transaction unit for the start block
      auto tu = std::make_shared<TransactionUnit>();
      
      auto source = _reader->GetSource();
      tu->messagedata.assign(source.begin(), source.end());
      tu->messagelength = (uint16_t)source.size();
      tu->messagetype = MsgType_StartTransaction;
      tu->transactionid = (uint32_t)transactionID;
      tu->sequencenum = 0;

      tu->GetBlob(buffer);
      _sender->Send(buffer);

      while (1)
      {
         // Create a transaction unit for this block
         auto tu = std::make_shared<TransactionUnit>();

         // Read the first block from the file
         int read = _reader->Read(tu->messagedata);
         if (read == 0) break;

         tu->messagetype = MsgType_Data;
         tu->messagelength = (uint16_t)tu->messagedata.size();
         tu->transactionid = (uint32_t)transactionID;
         tu->sequencenum = sequenceNumber++;
         _manager.Add(tu);

         // Send this block to the server
         tu->GetBlob(buffer);
         _sender->Send(buffer);
      }

      // Create a transaction unit for the end block
      tu = std::make_shared<TransactionUnit>();
      tu->messagedata.assign(source.begin(), source.end());
      tu->messagelength = (uint16_t)source.size();
      tu->messagetype = MsgType_EndTransaction;
      tu->transactionid = (uint32_t)transactionID;
      tu->sequencenum = 0;

      tu->GetBlob(buffer);
      _sender->Send(buffer);
   }
   catch (std::exception& e)
   {
      _logger->Log(5, e.what());
   }
}