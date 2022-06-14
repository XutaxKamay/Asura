#ifndef XKLIB_RUNNABLETASK_H
#define XKLIB_RUNNABLETASK_H

#include "buffer.h"
#include "exception.h"
#include "memoryutils.h"
#include "task.h"

namespace XKLib
{
    template <std::size_t N>
    class RunnableTask : public Task
    {
      public:
        RunnableTask(const ProcessBase processBase,
                     const ptr_t routineAddress);

      public:
        const auto& routineAddress() const;

        auto kill() const -> void;
        auto wait() const -> void;

        const auto& baseStack() const;
        auto freeStack() const -> void;

      public:
        auto& routineAddress();

        template <bool untraced>
        auto run() -> void;

      public:
        ptr_t _base_stack;

      private:
        ptr_t _routine_address;
#ifdef WINDOWS
        HANDLE _thread_handle;
#endif
    };

    template <std::size_t N>
    RunnableTask<N>::RunnableTask(ProcessBase processBase,
                                  ptr_t routineAddress)
     : Task(processBase),
       _routine_address(routineAddress)
#ifdef WINDOWS
       ,
       _thread_handle(nullptr)
#endif
    {
        static_assert(N >= 0x2000, "Stack size not large enough");

        _base_stack = MemoryUtils::AllocArea(
          _process_base.id(),
          nullptr,
          N,
          MemoryArea::ProtectionFlags::RW);
    }

    template <std::size_t N>
    const auto& RunnableTask<N>::routineAddress() const
    {
        return _routine_address;
    }

    template <std::size_t N>
    auto RunnableTask<N>::kill() const -> void
    {
#ifdef WINDOWS
        if (not _thread_handle)
        {
            XKLIB_EXCEPTION("Thread did not start yet");
        }

        if (not TerminateThread(_thread_handle, EXIT_CODE))
        {
            XKLIB_EXCEPTION("Could not terminate task");
        }

        CloseHandle(_thread_handle);
#else
        const auto ret = ::kill(_id, SIGKILL);

        if (ret != 0)
        {
            XKLIB_EXCEPTION("Could not terminate task "
                            + std::to_string(_id));
        }
#endif
    }

    template <std::size_t N>
    auto RunnableTask<N>::wait() const -> void
    {
#ifdef WINDOWS
        if (not _thread_handle)
        {
            XKLIB_EXCEPTION("Task did not start yet");
        }

        WaitForSingleObject(_thread_handle, INFINITE);

        CloseHandle(_thread_handle);
#else
        siginfo_t siginfo;
        const auto ret = waitid(P_PID, _id, &siginfo, WEXITED);

        if (ret < 0)
        {
            XKLIB_EXCEPTION("Could not wait for task "
                            + std::to_string(_id));
        }
#endif
    }

    template <std::size_t N>
    const auto& RunnableTask<N>::baseStack() const
    {
        return _base_stack;
    }

    template <std::size_t N>
    auto RunnableTask<N>::freeStack() const -> void
    {
        MemoryUtils::FreeArea(_process_base.id(), _base_stack, N);
    }

    template <std::size_t N>
    template <bool untraced>
    auto RunnableTask<N>::run() -> void
    {
#ifdef WINDOWS
        const auto process_handle = OpenProcess(
          PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
            | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
          false,
          _process_base.id());

        if (not process_handle)
        {
            XKLIB_EXCEPTION("Could not get permissions to create a "
                            "new "
                            "task");
        }

        _thread_handle = CreateRemoteThread(
          process_handle,
          0,
          N,
          view_as<LPTHREAD_START_ROUTINE>(_routine_address),
          0,
          0,
          view_as<PDWORD>(&_id));

        if (_thread_handle == nullptr)
        {
            _id = INVALID_ID;
            XKLIB_EXCEPTION("Could not create task");
        }

        CloseHandle(process_handle);
#else
        constexpr auto untraced_flag = []()
        {
            if constexpr (untraced)
            {
                return CLONE_UNTRACED;
            }

            return 0;
        }();

        _id = syscall(__NR_rclone,
                      _process_base.id(),
                      CLONE_VM | CLONE_SIGHAND | SIGCHLD | untraced_flag,
                      _routine_address,
                      view_as<std::uintptr_t>(_base_stack) - N
                        + MemoryUtils::GetPageSize(),
                      N);

        if (_id <= INVALID_ID)
        {
            XKLIB_EXCEPTION("Could not create task");
        }
#endif
    }

    template <std::size_t N>
    auto& RunnableTask<N>::routineAddress()
    {
        return _routine_address;
    }
};

#endif
