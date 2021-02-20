#include "FileReader.h"

FileReader::FileReader(std::shared_ptr<ILogger> logger)
   : _logger(logger),
   _blockSize(128)
{}

uint32_t FileReader::Read(std::vector<char>& s)
{
   // Open and read file data
   s.resize(_blockSize);
   _fileStream.read(s.data(), s.size());

   auto count = _fileStream.gcount();
   s.resize((size_t)count);
   return (uint32_t)count;
};

void FileReader::SetFile(const std::string& filename)
{
   _fileStream.open(filename);
}