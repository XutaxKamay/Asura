#ifndef RUNNABLETASK_H
#define RUNNABLETASK_H

#include "buffer.h"
#include "task.h"
#include "exception.h"

#ifdef WINDOWS
    #include <tlhelp32.h>
#else
    #include <cstdlib>
    #include <filesystem>
    #include <signal.h>
    #include <sys/wait.h>
#endif

namespace XLib
{
    template <size_t stack_size_T>
    class RunnableTask : public Task
    {
      public:
        RunnableTask(ProcessBase processBase, ptr_t routineAddress);

      public:
        auto kill() -> void;
        auto wait() -> void;

      public:
        auto run() -> void;

      private:
        ptr_t _routine_address;
#ifdef WINDOWS
        HANDLE _thread_handle;
#endif
    };

    template <size_t stack_size_T>
    RunnableTask<stack_size_T>::RunnableTask(ProcessBase processBase,
                                             ptr_t routineAddress)
     : Task(processBase), _routine_address(routineAddress)
#ifdef WINDOWS
       ,
       _thread_handle(nullptr)
#endif
    {
    }

    template <size_t stack_size_T>
    auto RunnableTask<stack_size_T>::run() -> void
    {
#ifdef WINDOWS
        auto process_handle = OpenProcess(
          PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
            | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
          false,
          _process_base.id());

        if (!process_handle)
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Could not get permissions to create a "
                                  "new "
                                  "thread");
        }

        _thread_handle = CreateRemoteThread(
          process_handle,
          0,
          stack_size_T,
          (LPTHREAD_START_ROUTINE)_routine_address,
          0,
          0,
          view_as<PDWORD>(&_id));

        if (_thread_handle == nullptr)
        {
            _id = INVALID_ID;
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Could not create thread");
        }

        CloseHandle(process_handle);
#else
        /**
         * CLONE_PARENT would be optional.
         * This is the minimalist:
         * (CLONE_VM | CLONE_SIGHAND | CLONE_THREAD)
         *
         * ( CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM
         * CLONE_SIGHAND | CLONE_THREAD
         * CLONE_SETTLS | CLONE_PARENT_SETTID
         * CLONE_CHILD_CLEARTID
         * 0)
         *
         */
        auto base_stack = view_as<ptr_t>(
          syscall(__NR_rmmap,
                  _process_base.id(),
                  0,
                  stack_size_T,
                  PROT_EXEC | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS,
                  0,
                  0));

        if (base_stack == nullptr)
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Could not allocate stack for the "
                                  "thread");
        }

        _id = syscall(__NR_rclone,
                      _process_base.id(),
                      (CLONE_VM | CLONE_SIGHAND | CLONE_THREAD),
                      _routine_address,
                      reinterpret_cast<ptr_t>(
                        reinterpret_cast<uintptr_t>(base_stack)
                        + stack_size_T - sizeof(ptr_t)),
                      stack_size_T);

        if (_id == INVALID_ID)
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Could not create thread");
        }
#endif
    }

    template <size_t stack_size_T>
    auto RunnableTask<stack_size_T>::kill() -> void
    {
#ifdef WINDOWS
        if (!_thread_handle)
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Thread did not start yet");
        }

        if (!TerminateThread(_thread_handle, EXIT_CODE))
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Could not terminate thread");
        }

        CloseHandle(_thread_handle);
#else
        auto ret = ::kill(_id, SIGKILL);

        if (ret != 0)
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Could not terminate thread");
        }
#endif
    }

    template <size_t stack_size_T>
    auto RunnableTask<stack_size_T>::wait() -> void
    {
#ifdef WINDOWS
        if (!_thread_handle)
        {
            throw Exception(std::string(CURRENT_CONTEXT)
                                + "Thread did not start yet");
        }

        WaitForSingleObject(_thread_handle, INFINITE);

        CloseHandle(_thread_handle);
#else
        while (::kill(_id, 0) != -1)
        {
            usleep(100);
        }
#endif
    }

};

#endif
