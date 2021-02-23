#pragma once

#include <string>

class ILogger
{
public:
   virtual void Log(int level, const std::string& s) = 0;
};

