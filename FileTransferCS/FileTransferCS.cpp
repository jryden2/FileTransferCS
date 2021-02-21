// FileTransferCS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 
// Winsock2 - Basic setup for UDP sockets pulled from various examples in https://docs.microsoft.com/en-us/windows/win32/api/winsock/
//

#include "UDPReceiver.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "UDPUnreliableSender.h"
#include "FileTransferClient.h"
#include "FileTransferServer.h"

#include <sstream>
#include <iostream>
#include <memory>

#include "WorkerThreadPool.h"
#include "SimpleLogger.h"


int main()
{
   auto logger = std::make_shared<SimpleLogger>();
   auto threadPool = std::make_shared<WorkerThreadPool>();
   auto receiver = std::make_shared<UDPReceiver>(logger, threadPool);
   auto sender = std::make_shared<UDPUnreliableSender>(logger);

   auto reader = std::make_shared<FileReader>(logger);
   reader->SetFile("Test.txt");

   auto writerFactory = std::make_shared<FileWriterFactory>();

   auto pFTS = std::make_unique<DataTransferServer>(logger, threadPool, receiver, writerFactory);
   auto pFTC = std::make_unique<DataTransferClient>(logger, threadPool, reader, sender);

   // Todo: Hang out for a while waiting for retransmit requests
   // Todo: Create lifetime control for the fts and/or ftc objects, for now just wait 
   //       for user input and terminate
   char q;
   std::cin >> q;

   std::cout << "Terminating processes..." << std::endl;
}
