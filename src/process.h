#ifndef PROCESS_H
#define PROCESS_H

#include "memorymap.h"
#include "memoryutils.h"
#include "processbase.h"
#include "exception.h"
#include "processmemoryarea.h"
#include "processmemorymap.h"
#include "runnabletask.h"

namespace XLib
{
    class Process : public ProcessBase
    {
      public:
        static inline constexpr pid_t INVALID_PID = -1;

      public:
        static auto self() -> Process;
        static auto find(const std::string& name) -> Process;
        static auto ProcessName(pid_t pid) -> std::string;

      public:
        Process();
        Process(pid_t pid);

      public:
        auto tasks() -> tasks_t;
        auto mmap() -> ProcessMemoryMap;

      public:
        template <size_t stack_size_T = 0x10000>
        auto createTask(ptr_t routineAddress)
          -> RunnableTask<stack_size_T>
        {
            return RunnableTask<stack_size_T>(*this, routineAddress);
        }

        template <typename T = uintptr_t>
        auto allocArea(T address, size_t size, mapf_t flags) -> ptr_t
        {
            return mmap().allocArea<T>(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size) -> void
        {
            mmap().freeArea<T>(address, size);
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address, size_t size, mapf_t flags)
          -> void
        {
            mmap().protectMemoryArea(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto read(T address, size_t size) -> bytes_t
        {
            return mmap().read(address, size);
        }

        template <typename T = uintptr_t>
        auto write(T address, bytes_t bytes) -> void
        {
            mmap().write(address, bytes);
        }

        template <typename T = uintptr_t>
        auto forceWrite(T address, bytes_t bytes) -> void
        {
            mmap().forceWrite(address, bytes);
        }

      private:
        std::string _full_name {};
        ProcessMemoryMap _mmap;
    };
}

#endif
