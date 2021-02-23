#pragma once

#include <vector>
#include <string>

class IReader
{
public:
   virtual uint32_t Read(std::vector<char>& s) = 0;
   virtual const std::string& GetSource() = 0;
};
