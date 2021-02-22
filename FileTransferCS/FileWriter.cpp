#include "FileWriter.h"

std::shared_ptr<IWriter> FileWriterFactory::Create(std::shared_ptr<ILogger> logger)
{
   return std::make_shared<FileWriter>(logger);
}

FileWriter::FileWriter(std::shared_ptr<ILogger> logger)
   : _logger(logger)
{

}

FileWriter::~FileWriter()
{
   if (_file.is_open())
   {
      _file.close();
   }
}
void FileWriter::SetDestination(const std::string& s)
{
   _filename = s;

   // Todo: Error handling
   _file.open(_filename);
}

void FileWriter::Write(const std::string& s)
{
   if (_file.is_open())
   {
      _file << s;
   }
}
