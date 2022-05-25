#ifndef XKLIB_PROCESS_H
#define XKLIB_PROCESS_H

#include "exception.h"
#include "memoryarea.h"
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
        class Module
        {
          public:
            Module() = default;
            Module(ptr_t baseAddress,
                   const std::string& name,
                   const std::string& path = {});

          public:
            auto baseAddress() const -> const ptr_t&;
            auto name() const -> const std::string&;
            auto path() const -> const std::string&;

          public:
            auto baseAddress() -> ptr_t&;
            auto name() -> std::string&;
            auto path() -> std::string&;

          private:
            ptr_t _base_address;
            std::string _name;
            std::string _path;
        };

      public:
        static inline constexpr process_id_t INVALID_PID    = -1;
        static inline constexpr std::size_t TASK_STACK_SIZE = 0x100000;

      public:
        static auto self() -> Process;
        static auto find(const std::string& name) -> Process;
        static auto name(const process_id_t pid)
          -> std::tuple<std::string, bool>;

      public:
        Process();
        explicit Process(const process_id_t pid);

      public:
        auto tasks() const -> tasks_t;
        auto mmap() const -> const ProcessMemoryMap&;
        auto modules() const -> const std::list<Module>&;
        auto search(PatternByte& patternByte) const -> void;

      public:
        auto mmap() -> ProcessMemoryMap&;
        auto modules() -> std::list<Module>&;
        auto refreshModules() -> void;

      public:
        template <std::size_t N = TASK_STACK_SIZE>
        auto createTask(const ptr_t routineAddress) -> RunnableTask<N>
        {
            return RunnableTask<N>(*this, routineAddress);
        }

        auto read(const auto address, const std::size_t size) const
          -> bytes_t
        {
            return _mmap.read(address, size);
        }

        auto write(const auto address, const bytes_t& bytes) const -> void
        {
            _mmap.write(address, bytes);
        }

        auto write(const auto address,
                   const auto ptr,
                   const std::size_t size) const -> void
        {
            _mmap.write(address, ptr, size);
        }

      public:
        auto allocArea(const auto address,
                       const std::size_t size,
                       const mapf_t flags) -> ptr_t
        {
            return _mmap.allocArea<decltype(address)>(address,
                                                      size,
                                                      flags);
        }

        auto freeArea(const auto address, const std::size_t size) -> void
        {
            _mmap.freeArea<decltype(address)>(address, size);
        }

        auto protectMemoryArea(const auto address,
                               const std::size_t size,
                               const mapf_t flags) -> void
        {
            _mmap.protectMemoryArea(address, size, flags);
        }

        auto forceWrite(const auto address, const bytes_t& bytes) -> void
        {
            _mmap.forceWrite(address, bytes);
        }

        auto forceWrite(const auto address,
                        const auto ptr,
                        const std::size_t size) -> void
        {
            _mmap.forceWrite(address, ptr, size);
        }

      private:
        std::string _full_name;
        ProcessMemoryMap _mmap;
        std::list<Module> _modules;
    };
}

#endif
