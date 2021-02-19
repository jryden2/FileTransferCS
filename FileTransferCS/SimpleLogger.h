#pragma once

#include "ILogger.h"

#include <iostream>
#include <sstream>

class SimpleLogger : public ILogger
{
public:
   void Log(int level, const std::string& s) override
   {
      std::stringstream ss;
      ss << "[Sev:" << level << "] " << s << std::endl;
      std::cout << ss.str();

#ifdef _WIN32
      OutputDebugString(ss.str().c_str());
#endif
   }
};
