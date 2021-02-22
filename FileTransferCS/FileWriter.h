#pragma once

#include "ILogger.h"
#include "IWriter.h"

#include <fstream>

class FileWriterFactory : public IWriterFactory
{
public:
   std::shared_ptr<IWriter> Create(std::shared_ptr<ILogger> logger) override;
};

class FileWriter : public IWriter
{
public:
   FileWriter(std::shared_ptr<ILogger> logger);
   ~FileWriter();

   void Write(const std::string& s) override;
   const std::string& GetDestination() override { return _filename; }
   void SetDestination(const std::string& s) override;

private:
   std::shared_ptr<ILogger> _logger;
   std::ofstream _file;
   std::string _filename;
};

