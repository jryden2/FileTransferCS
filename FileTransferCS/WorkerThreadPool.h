#pragma once

#include "IWorkerThreadPool.h"

#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>
#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

class WorkerThreadPool : public IWorkerThreadPool
{
public:
   WorkerThreadPool()
      : _stopFlag(false),
      _threadCount(2)
   {
      printf("WorkerThread construct");
      Start();
   }

   virtual ~WorkerThreadPool()
   {
      Stop();
   }


   /*
    * ===  FUNCTION  ======================================================================
    *         Name:  Start
    *  Description: Todo, de-uglify this comment template
    *  Todo: Expand to more than a single thread
    * =====================================================================================
    */
   virtual void Start() override
   {
      _thread = std::thread([=]()
      {
         while (true)
         {
            // Construct a lock object and wait for a client to put a request on the queue
            std::unique_lock<std::mutex> lk(_mutex);

            if (!_timerMap.empty())
            {
               // If there is a timer request in the queue, start a timed wait for the target time of the first entry
               auto targetTime = _timerMap.begin()->first;

               // wait_until will block until the target time is reached.  The predicate lambda checks for spurious
               // wakeups
               _conditionVariable.wait_until(lk, targetTime, [=]() { return !_taskQueue.empty(); });
            }
            else
            {
               // Start an untimed wait.
               _conditionVariable.wait(lk, [=]() { return (!_taskQueue.empty() || !_timerMap.empty()); });
            }

            // If the stop signal is set, break out and terminate
            if (_stopFlag) break;

            // Check for an expired timer
            if (!_timerMap.empty())
            {
               auto iter = _timerMap.begin();
               auto targetTime = iter->first;
               auto callbackMethod = iter->second;
               //if (targetTime < std::chrono::system_clock::now())
               {
                  _timerMap.erase(iter);

                  // Release the lock before executing the callback method
                  lk.unlock();

                  // Execute the callback which is stored in the value (second) of the map pair
                  callbackMethod();
               }
               // Note that only one timer is removed from the map, there could be mulitple timers that are firing at the same
               // time.  But we return control to the thread now and let it expire again.  This way, other threads may be
               // able to process timers and events
            }
            else if (!_taskQueue.empty())
            {
               // Pop a copy of the next task off the queue
               auto Task = _taskQueue.front();
               _taskQueue.pop();

               // Release the lock before executing the task Task() is the stored function
               lk.unlock();
               Task();
            }
         }
      });
   }

   void Stop() override
   {
      if (!_stopFlag)
      {
         _stopFlag = true;

         // Post a 0 task to wake the thread.  
         Post(0);

         // Wait for the thread
         _thread.join();
      }
   }

   void StartTimer(int timeoutMs, std::function<void()> Callback) override
   {
      // Register a timer
      bool wakeTimerThread = false;

      {
         // Lock the mutex while we push a new task onto the queue
         std::lock_guard<std::mutex> lk(_mutex);

         // Create an absolute target time for this timer entry
         auto now = std::chrono::system_clock::now();
         auto target = now + std::chrono::milliseconds(timeoutMs);

         //if (_timerMap.empty() || _timerMap.begin()->first > target)
         //{
         //    wakeTimerThread = true;
         //}
         //
         //_timerMap.insert(std::make_pair(target, Callback));
      }

      if (wakeTimerThread)
      {
         // Wake up a thread to handle the new timer
         _conditionVariable.notify_one();
      }

      printf("Start timer req: %s, %ld\n", wakeTimerThread ? "wake" : "nowake", _timerMap.size());
   }

   // See IWorkerThreadPool.h for details on stands
   void CreateStrand(int& nStrandID) override {}
   void DestroyStrand(int nStrandID) override {}

   /// ////////////////////////////////////////////////////////////////////////////////////////////////
   /// Post a TASK to the task queue.
   /// ////////////////////////////////////////////////////////////////////////////////////////////////
   void Post(std::function<void()> Task) override
   {
      {
         // Lock the mutex while we push a new task onto the queue
         std::lock_guard<std::mutex> lk(_mutex);
         _taskQueue.push(Task);
      }
      _conditionVariable.notify_one();
   }

   void Post(std::function<void()> Task, int nStrandID) override
   {
      // Strand execution will apply a strand id to a task
      Post(Task);
   }

private:
   int _threadCount;
   std::queue<std::function<void()>> _taskQueue;
   std::map<TimePoint, std::function<void()>> _timerMap;
   std::thread _thread;
   bool _stopFlag;
   std::mutex _mutex;
   std::condition_variable _conditionVariable;
};
