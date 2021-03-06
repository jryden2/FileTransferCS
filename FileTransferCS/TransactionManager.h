#pragma once

#include <memory>
#include <map>

#include "TransactionUnit.h"

class TransactionManager
{
public:
   TransactionManager() = default;
   ~TransactionManager() = default;

   void Add(std::shared_ptr<TransactionUnit> tu)
   {
      // Find the associated transaction and add the unit to the list of pending messages
      auto iter = _packetMap.find(tu->transactionid);
      if (iter != _packetMap.end())
      {
         iter->second.insert(std::make_pair(tu->sequencenum, tu));
      }
      else
      {
         // Create a new transaction for this unit
         _packetMap[tu->transactionid].insert(std::make_pair(tu->sequencenum, tu));
      }
   }

   std::shared_ptr<TransactionUnit> Collect(uint32_t transactionID)
   {
      auto iter = _packetMap.find(transactionID);
      if (iter != _packetMap.end())
      {
         auto& sequenceMap = iter->second;
         if (sequenceMap.size())
         {
            auto p = sequenceMap.begin()->second;

            // Check sequence
            if (p->sequencenum == _nextSequenceMap[transactionID])
            {
               _nextSequenceMap[transactionID]++;
               _packetMap.erase(iter);
               return p;
            }
         }
      }

      return std::shared_ptr<TransactionUnit>();
   }

private:
   std::map<uint32_t, std::map<uint32_t, std::shared_ptr<TransactionUnit>>> _packetMap;
   std::map<uint32_t, uint32_t> _nextSequenceMap;
};


