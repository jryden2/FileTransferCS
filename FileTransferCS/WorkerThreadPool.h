#pragma once

#include "IWorkerThreadPool.h"

#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>

class WorkerThreadPool : public IWorkerThreadPool
{
public:
    WorkerThreadPool() :
        _stopFlag(false),
	_threadCount(2) {}

    virtual ~WorkerThreadPool() 
    {
        Stop();
    }

    virtual void Start() override
    {
       _thread = std::thread([&]()
       {
          while (true)
          {
             // Construct a lock object and wait for a client to put a request on the queue
             std::unique_lock<std::mutex> lk(_mutex);
             _conditionVariable.wait(lk, [=] {return !_taskQueue.empty(); });

             // If the stop signal is set, break out and terminate
             if (_stopFlag) break;

             // Pop a copy of the next task off the queue
             auto Task = _taskQueue.front();
             _taskQueue.pop();

             // Release the lock before executing the task Task() is the stored function
             lk.unlock();
             Task();
          }
       });
    }

    virtual void Stop() override
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

    // Timer methods not implemented, but could be later if needed
    bool StartTimer(std::function<void()> CallBack) override 
    {
	// Register a timer
	
    }

    const size_t& GetThreadCount(void) const override { return _threadCount; }

    // Strands have no effect for this single thread implementation
    // See IWorkerThreadPool.h for details on stands
    void CreateStrand(int& nStrandID) override {}
    void DestroyStrand(int nStrandID) override {}

    /// ////////////////////////////////////////////////////////////////////////////////////////////////
    /// Post a TASK to the task queue.
    /// ////////////////////////////////////////////////////////////////////////////////////////////////
    void Post(std::function<void>() Task) override
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
    std::thread _thread;
    bool _stopFlag;
    std::mutex _mutex;
    std::condition_variable _conditionVariable;
};
