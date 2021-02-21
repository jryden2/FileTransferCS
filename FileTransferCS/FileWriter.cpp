#include "FileWriter.h"

std::shared_ptr<IWriter> FileWriterFactory::Create()
{
   return std::make_shared<FileWriter>();
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
