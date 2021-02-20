#pragma once

#include <vector>

class IReader
{
public:
   virtual uint32_t Read(std::vector<char>& s) = 0;
};
