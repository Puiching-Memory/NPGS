#if defined(__linux__)
#   define _GNU_SOURCE
#endif
#include "ThreadPool.h"

#include <cstdint>
#include <cstdlib>

#if defined(_WIN32)
#   include <Windows.h>
#elif defined(__linux__)
#   include <pthread.h>
#endif

_NPGS_BEGIN
_RUNTIME_BEGIN
_THREAD_BEGIN

namespace
{
    int GetPhysicalCoreCount()
    {
#if defined(_WIN32)
        DWORD Length = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &Length);
        std::vector<std::uint8_t> Buffer(Length);
        auto* BufferPtr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(Buffer.data());
        GetLogicalProcessorInformationEx(RelationProcessorCore, BufferPtr, &Length);

        int CoreCount = 0;
        while (Length > 0)
        {
            if (BufferPtr->Relationship == RelationProcessorCore)
            {
                ++CoreCount;
            }

            Length -= BufferPtr->Size;
            BufferPtr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                reinterpret_cast<std::uint8_t*>(BufferPtr) + BufferPtr->Size);
        }

        return CoreCount;
#else
        const unsigned int Count = std::thread::hardware_concurrency();
        return Count == 0 ? 1 : static_cast<int>(Count);
#endif
    }
}

// ThreadPool implementations
// --------------------------
FThreadPool::FThreadPool()
    :
    _Terminate(false),
    _kHyperThreadIndex(0),
    _kMaxThreadCount(GetPhysicalCoreCount()),
    _kPhysicalCoreCount(GetPhysicalCoreCount())
{
    for (std::size_t i = 0; i != _kPhysicalCoreCount; ++i)
    {
        _Threads.emplace_back([this]() -> void
        {
            while (true)
            {
                std::function<void()> Task;
                {
                    std::unique_lock<std::mutex> Mutex(_Mutex);
                    _Condition.wait(Mutex, [this]() -> bool { return !_Tasks.empty() || _Terminate; });
                    if (_Terminate && _Tasks.empty())
                    {
                        return;
                    }

                    Task = std::move(_Tasks.front());
                    _Tasks.pop();
                }

                Task();
            }
        });

        SetThreadAffinity(_Threads.back(), i);
    }
}

FThreadPool::~FThreadPool()
{
    Terminate();
}

void FThreadPool::Terminate()
{
    {
        std::unique_lock<std::mutex> Mutex(_Mutex);
        _Terminate = true;
    }
    _Condition.notify_all();
    for (auto& Thread : _Threads)
    {
        if (Thread.joinable())
        {
            Thread.join();
        }
    }
}

FThreadPool* FThreadPool::GetInstance()
{
    static FThreadPool kInstance;
    return &kInstance;
}

void FThreadPool::SetThreadAffinity(std::thread& Thread, std::size_t CoreId) const
{
#if defined(_WIN32)
    HANDLE Handle = Thread.native_handle();
    DWORD_PTR Mask = 0;
    Mask = static_cast<DWORD_PTR>(Bit(CoreId * 2) + _kHyperThreadIndex);
    SetThreadAffinityMask(Handle, Mask);
#elif defined(__linux__)
    cpu_set_t CpuSet;
    CPU_ZERO(&CpuSet);
    CPU_SET(static_cast<int>(CoreId % _kPhysicalCoreCount), &CpuSet);
    pthread_setaffinity_np(Thread.native_handle(), sizeof(CpuSet), &CpuSet);
#endif
}

_THREAD_END
_RUNTIME_END
_NPGS_END
