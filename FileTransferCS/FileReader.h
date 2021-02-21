#pragma once

#include "IReader.h"
#include "ILogger.h"

#include <memory>
#include <fstream>

class FileReader : public IReader
{
public:
   FileReader(std::shared_ptr<ILogger> logger);
   FileReader(const FileReader&) = delete;
   FileReader(FileReader&&) = default;
   ~FileReader() = default;


   uint32_t Read(std::vector<char>& s) override;
   const std::string& GetSource() override { return _filename; }

   void SetFile(const std::string& filename);

private:
   std::shared_ptr<ILogger> _logger;
   std::fstream _fileStream;
   std::string _filename;
   const uint8_t _blockSize;
};
