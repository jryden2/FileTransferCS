#pragma once

#include <functional>
#include <vector>

class IReceiver
{
public:
   virtual void Receive(std::function<void(std::vector<char>)> callback) = 0;
};
