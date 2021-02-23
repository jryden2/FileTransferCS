#pragma once

#include <functional>

/// ////////////////////////////////////////////////////////////////////////////////////////////////
/// Worker thread pool. Accepts arbitrary tasks from the application and processes
/// them with the available threads. 
/// ////////////////////////////////////////////////////////////////////////////////////////////////
class IWorkerThreadPool
{
public:
    IWorkerThreadPool() {};
    virtual ~IWorkerThreadPool() {};

    virtual void SetThreadCount(int count) = 0;
    virtual void Stop() = 0;
   
    virtual void StartTimer(int timeoutMs, std::function<void()> callback) = 0;
    
    virtual void CreateStrand(int& nStrandID) = 0;
    virtual void DestroyStrand(int nStrandID) = 0;
   
    virtual void Post(std::function<void()> Task) = 0;
    virtual void Post(std::function<void()> Task, int nStrandID) = 0;
};

