
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
class MockSenderReceiver : public ISenderReceiver
{
public:
	void Send(const std::shared_ptr<TransactionUnit>& pTu)
	{
		std::string stringData;
		pTu->GetMessageData(stringData);
		_sendData.push_back(stringData);
	}

	void Receive(std::function<void(std::shared_ptr<TransactionUnit>)> callback) { _callback = callback; }

	void Start(uint16_t port)
	{}

	std::vector<std::string> _sendData;
	std::function<void(std::shared_ptr<TransactionUnit>)> _callback;
};
class MockWriter : public IWriter
{
public:
	void Write(const std::string& s) override { _writes.push_back(s); }
	const std::string& GetDestination() override { return _dest; }
	void SetDestination(const std::string& s) override { _dest = s; }

	std::string _dest;
	std::vector<std::string> _writes;
};
class MockWriterFactory : public IWriterFactory
{
public:
	std::shared_ptr<IWriter> Create(std::shared_ptr<ILogger> logger) override
	{
		if (_mock) return _mock;

		_mock = std::make_shared<MockWriter>();
		return _mock; 
	}

	std::shared_ptr<MockWriter> _mock;
};

namespace UnitTest
{
	TEST_CLASS(UnitTest)
	{
	public:
		
		TEST_METHOD(DataTransferClient_SendData)
		{
			auto loggerStub = std::make_shared<LoggerStub>();
			auto threadPool = std::make_shared<WorkerThreadPool>();
			auto reader = std::make_shared<MockReader>();
			auto sender = std::make_shared<MockSenderReceiver>();
			
			std::string sReaderData = reader->data;
			std::string sReaderSource = reader->source;
			
			auto p = std::make_shared<DataTransferClient>(loggerStub, threadPool, reader, sender);

			auto actualData = (int)sender->_sendData.size();
			Assert::AreEqual(3, actualData);
		}

		TEST_METHOD(DataTransferServer_Start_SingleThread)
		{
			auto loggerStub = std::make_shared<LoggerStub>();
			auto threadPool = std::make_shared<WorkerThreadPool>();
			threadPool->SetThreadCount(1);
			auto writer = std::make_shared<MockWriterFactory>();
			auto receiver = std::make_shared<MockSenderReceiver>();

			auto pServer = std::make_shared<DataTransferServer>(loggerStub, threadPool, receiver, writer);

			// We expect that that server will have registered for receive callbacks by now...
			Assert::IsTrue((bool)receiver->_callback);

			if (receiver->_callback)
			{
				// Execute the call back with some data
				auto pTu = std::make_shared<TransactionUnit>();
				pTu->SetMessageType(MsgType_StartTransaction);
				pTu->SetSequenceNumber(0);

				std::string destination("Test Message Data 12345");
				pTu->SetMessageData(destination);
				receiver->_callback(pTu);

				Sleep(5000);
				threadPool->Stop();
				
				// After a start call the factory should have created a mock writer for the server
				Assert::IsTrue(writer->_mock.get() != 0);

				// Also, the destination we used should have been set as the destination in the mock writer
				if (writer->_mock)
				{
					Assert::AreEqual(destination, writer->_mock->GetDestination());
				}
			}
		}
	};
}
