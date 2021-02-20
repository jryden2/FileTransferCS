
#include "CppUnitTest.h"

#include <iostream>

#include "..\FileTransferCS\FileReader.h"
#include "..\FileTransferCS\ILogger.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class LoggerStub : public ILogger
{
	void Log(int level, const std::string& s) 
	{
		std::cout << s;
	}
};

namespace UnitTest
{
	TEST_CLASS(UnitTest)
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
	};
}
