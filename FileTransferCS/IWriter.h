#pragma once

#include <memory>
#include <string>

class IWriter
{
public:
   virtual void Write(const std::string& s) = 0;
   virtual const std::string& GetDestination() = 0;
   virtual void SetDestination(const std::string& s) = 0;
};

class IWriterFactory
{
public:
   virtual std::shared_ptr<IWriter> Create() = 0;
};

