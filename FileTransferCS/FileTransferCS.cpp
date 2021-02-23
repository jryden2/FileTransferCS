// FileTransferCS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 
// Winsock2 - Basic setup for UDP sockets pulled from various examples in https://docs.microsoft.com/en-us/windows/win32/api/winsock/
//

#include "FileReader.h"
#include "FileWriter.h"
#include "UDPUnreliableSenderReceiver.h"
#include "DataTransferClient.h"
#include "DataTransferServer.h"

#include <sstream>
#include <iostream>
#include <memory>

#include "WorkerThreadPool.h"
#include "SimpleLogger.h"


int main(int argc, char* argv[])
{
   std::string filename("Test.txt");

   bool bServer = true;
   bool bClient = true;
   for (int i = 1; i<argc; i++)
   {
      std::string s = argv[i];
      if (s == "--server")
      {
         bClient = false;
         break;
      }

      if (s == "--client")
      {
         bServer = false;
         break;
      }

      filename = argv[i];
   }

   auto logger = std::make_shared<SimpleLogger>();
   auto threadPool = std::make_shared<WorkerThreadPool>();
   threadPool->SetThreadCount(4);

   auto reader = std::make_shared<FileReader>(logger);
   reader->SetFile(filename);

   auto writerFactory = std::make_shared<FileWriterFactory>();

   std::unique_ptr<DataTransferServer> pFTS;
   if (bServer)
   {
      auto senderRecieverServer = std::make_shared<UDPUnreliableSenderReceiver>(logger, threadPool);
      senderRecieverServer->Start(1234);

      pFTS = std::make_unique<DataTransferServer>(logger, threadPool, senderRecieverServer, writerFactory);
   }

   std::unique_ptr<DataTransferClient> pFTC;
   if (bClient)
   {
      auto senderRecieverClient = std::make_shared<UDPUnreliableSenderReceiver>(logger, threadPool);
      senderRecieverClient->Start(0);

      pFTC = std::make_unique<DataTransferClient>(logger, threadPool, reader, senderRecieverClient);
   }

   // Todo: Hang out for a while waiting for retransmit requests
   // Todo: Create lifetime control for the fts and/or ftc objects, for now just wait 
   //       for user input and terminate
   char q;
   std::cin >> q;

   std::cout << "Terminating processes..." << std::endl;
}
