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
#include "FileTransferCS.h"


int main(int argc, char* argv[])
{
   std::string filename("Test.txt");

   bool bServer = true;
   bool bClient = true;
   ProcessCommandLine(argc, argv, bClient, bServer, filename);

   // Create various interfaces for injection
   auto logger = std::make_shared<SimpleLogger>();
   auto threadPool = std::make_shared<WorkerThreadPool>();
   threadPool->SetThreadCount(4);

   auto reader = std::make_shared<FileReader>(logger);
   reader->SetFile(filename);

   auto writerFactory = std::make_shared<FileWriterFactory>();

   // Create server
   std::unique_ptr<DataTransferServer> pFTS;
   if (bServer)
   {
      auto senderRecieverServer = std::make_shared<UDPUnreliableSenderReceiver>(logger, threadPool);
      senderRecieverServer->Start("127.0.0.1", 11111, 11112);

      pFTS = std::make_unique<DataTransferServer>(logger, threadPool, senderRecieverServer, writerFactory);
   }

   // Create client
   std::unique_ptr<DataTransferClient> pFTC;
   if (bClient)
   {
      auto senderRecieverClient = std::make_shared<UDPUnreliableSenderReceiver>(logger, threadPool);
      senderRecieverClient->Start("127.0.0.1", 11112, 11111);

      pFTC = std::make_unique<DataTransferClient>(logger, threadPool, reader, senderRecieverClient);
   }

   // Todo: Hang out for a while waiting for retransmit requests
   // Todo: Create lifetime control for the fts and/or ftc objects, for now just wait 
   //       for user input and terminate
   char q;
   std::cin >> q;

   std::cout << "Terminating processes..." << std::endl;
}

void ProcessCommandLine(int argc, char* argv[], bool& bClient, bool& bServer, std::string& filename)
{
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
}
