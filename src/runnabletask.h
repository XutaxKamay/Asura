#ifndef RUNNABLETASK_H
#define RUNNABLETASK_H

#include "buffer.h"
#include "task.h"

namespace XLib
{
    class RunnableTask : public Task
    {
      public:
        RunnableTask(ProcessBase* processBase, ptr_t routineAddress);

        auto run() -> void;

      private:
        ptr_t _routine_address;
        ptr_t _args;
        size_t _stack_size;
#ifdef WINDOWS
        HANDLE _thread_handle;
#endif
    };
};

#endif
