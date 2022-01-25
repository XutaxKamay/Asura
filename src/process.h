#ifndef PROCESS_H
#define PROCESS_H

#include "exception.h"
#include "memorymap.h"
#include "memoryutils.h"
#include "patternbyte.h"
#include "processbase.h"
#include "processmemoryarea.h"
#include "processmemorymap.h"
#include "runnabletask.h"

namespace XKLib
{
    class Process : public ProcessBase
    {
      public:
        static inline constexpr process_id_t INVALID_PID    = -1;
        static inline constexpr std::size_t TASK_STACK_SIZE = 0x10000;

      public:
        static auto self() -> Process;
        static auto find(const std::string& name) -> Process;
        static auto ProcessName(process_id_t pid) -> std::string;

      public:
        Process();
        explicit Process(process_id_t pid);

      public:
        auto tasks() -> tasks_t;
        auto mmap() -> ProcessMemoryMap;
        auto search(PatternByte& patternByte) -> void;

      public:
        template <std::size_t stack_size_T = TASK_STACK_SIZE>
        auto createTask(ptr_t routineAddress)
          -> RunnableTask<stack_size_T>
        {
            return RunnableTask<stack_size_T>(*this, routineAddress);
        }

        template <typename T = uintptr_t>
        auto allocArea(T address, std::size_t size, mapf_t flags) -> ptr_t
        {
            return mmap().allocArea<T>(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, std::size_t size) -> void
        {
            mmap().freeArea<T>(address, size);
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address, std::size_t size, mapf_t flags)
          -> void
        {
            mmap().protectMemoryArea(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto read(T address, std::size_t size) -> bytes_t
        {
            return mmap().read(address, size);
        }

        template <typename T = uintptr_t>
        auto write(T address, const bytes_t& bytes) -> void
        {
            mmap().write(address, bytes);
        }

        template <typename T = uintptr_t>
        auto forceWrite(T address, const bytes_t& bytes) -> void
        {
            mmap().forceWrite(address, bytes);
        }

        template <typename T = uintptr_t>
        auto write(T address, auto ptr, std::size_t size) -> void
        {
            mmap().write(address, ptr, size);
        }

        template <typename T = uintptr_t>
        auto forceWrite(T address, auto ptr, std::size_t size) -> void
        {
            mmap().forceWrite(address, ptr, size);
        }

      private:
        std::string _full_name {};
        ProcessMemoryMap _mmap;
    };
}

#endif
