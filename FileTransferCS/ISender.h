#pragma once

#include <vector>

class ISender
{
public:
   virtual void Send(const std::vector<char>& s) = 0;
};

