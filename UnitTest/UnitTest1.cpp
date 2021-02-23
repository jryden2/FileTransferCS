
#include "CppUnitTest.h"

#include <iostream>

#include "..\FileTransferCS\FileReader.h"
#include "..\FileTransferCS\FileWriter.h"
#include "..\FileTransferCS\ILogger.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class LoggerStub : public ILogger
{
	void Log(int level, const std::string& s) 
	{
		std::cout << s;
	}
};

namespace UnitTest1
{
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(FileReader_Read_SmallFile)
		{
			std::string s = "Test file 12345";
			std::string tempName = std::tmpnam(nullptr);
			std::ofstream f(tempName);
			f << s;
			f.close();

			auto p = std::make_shared<FileReader>(std::make_shared<LoggerStub>());
			p->SetFile(tempName);
			
			std::vector<char> buf;
			p->Read(buf);

			std::string sbuf(buf.begin(), buf.end());
			Assert::AreEqual(sbuf, s);
		}

		TEST_METHOD(FileWriter_Write_SmallFile)
		{
			std::string sWriteData = "Test file 12345";
			std::string tempName = std::tmpnam(nullptr);

			auto p = std::make_shared<FileWriter>(std::make_shared<LoggerStub>());
			p->SetDestination(tempName);
			p->Write(sWriteData);
			p.reset();

			std::ifstream f(tempName);
			std::vector<char> readData;
			readData.resize(sWriteData.size());
			f.read(readData.data(), sizeof(sWriteData));
			f.close();

			std::string sbuf(readData.begin(), readData.end());
			Assert::AreEqual(sbuf, sWriteData);
		}

	};
}
