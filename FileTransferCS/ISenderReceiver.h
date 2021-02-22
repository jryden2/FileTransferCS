#pragma once

#include <functional>
#include <vector>

class ISenderReceiver
{
public:
   virtual void Send(const std::vector<char>& s) = 0;
   virtual void Receive(std::function<void(std::vector<char>)> callback) = 0;
   virtual void Start(uint16_t port) = 0;
};
