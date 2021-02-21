#pragma once

#include "IWriter.h"

#include <fstream>

class FileWriterFactory : public IWriterFactory
{
public:
   std::shared_ptr<IWriter> Create() override;
};

class FileWriter : public IWriter
{
public:
   FileWriter() = default;
   ~FileWriter();

   void Write(const std::string& s) override;
   const std::string& GetDestination() override { return _filename; }
   void SetDestination(const std::string& s) override;

private:
   std::ofstream _file;
   std::string _filename;
};

