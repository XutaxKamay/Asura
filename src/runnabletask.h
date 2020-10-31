#ifndef RUNNABLETASK_H
#define RUNNABLETASK_H

#include "buffer.h"
#include "task.h"

namespace XLib
{
    template <size_t stack_size_T>
    class RunnableTask : public Task
    {
      public:
        RunnableTask(ProcessBase* processBase, ptr_t routineAddress);

        auto run() -> void;

      private:
        ptr_t _routine_address;
#ifdef WINDOWS
        HANDLE _thread_handle;
#endif
    };
};

#endif
