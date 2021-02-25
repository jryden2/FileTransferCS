#pragma once

#include <functional>
#include <vector>

#include "TransactionUnit.h"

class ISenderReceiver
{
public:
   virtual void Send(const std::shared_ptr<TransactionUnit>& tu) = 0;
   virtual void Receive(std::function<void(std::shared_ptr<TransactionUnit>)> callback) = 0;
};
