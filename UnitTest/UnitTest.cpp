
#include "CppUnitTest.h"

#include <iostream>
#include <string>

#include "..\FileTransferCS\ILogger.h"
#include "..\FileTransferCS\IReader.h"
#include "..\FileTransferCS\ISenderReceiver.h"

#include "..\FileTransferCS\WorkerThreadPool.h"

#include "..\FileTransferCS\DataTransferClient.h"
#include "..\FileTransferCS\DataTransferServer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class LoggerStub : public ILogger
{
	void Log(int level, const std::string& s) 
	{
		std::cout << s;
	}
};

class MockReader : public IReader
{
public:
	MockReader()
		: source("Test Source"),
		  data("Test Data 12345")
	{}

	uint32_t Read(std::vector<char>& s) override
	{
		s.assign(data.begin(), data.end());

		// Simulate reading from a file by clearing the data now.  Only the first call to this method
		// will return data
		data.clear();

		return s.size();
	}

	virtual const std::string& GetSource() override
	{
		return source;
	}

	std::string source;
	std::string data;
};

class MockSender : public ISenderReceiver
{
public:
	void Send(const std::vector<char>& s)
	{
		std::string stringData;
		stringData.assign(s.begin(), s.end());
		sendData.push_back(stringData);
	}

	void Receive(std::function<void(std::vector<char>)> callback)
	{}

	void Start(uint16_t port)
	{}

	std::vector<std::string> sendData;
};

namespace UnitTest
{
	TEST_CLASS(UnitTest)
	{
	public:
		
		TEST_METHOD(DataTransferClient_Basic)
		{
			auto loggerStub = std::make_shared<LoggerStub>();
			auto threadPool = std::make_shared<WorkerThreadPool>();
			auto reader = std::make_shared<MockReader>();
			auto sender = std::make_shared<MockSender>();
			
			std::string sReaderData = reader->data;
			std::string sReaderSource = reader->source;
			
			auto p = std::make_shared<DataTransferClient>(loggerStub, threadPool, reader, sender);

			auto actualData = (int)sender->sendData.size();
			Assert::AreEqual(3, actualData);
		}
	};
}
