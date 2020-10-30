#ifndef RUNNABLETASK_H
#define RUNNABLETASK_H

#include "buffer.h"
#include "task.h"

namespace XLib
{
    template <size_t arg_size_T = 0>
    class RunnableTask : public Task
    {
      public:
        RunnableTask(ProcessBase* processBase,
                     ptr_t routineAddress,
                     tid_t id,
                     Buffer<arg_size_T> arg);

        auto run() -> void;

      private:
        Buffer<arg_size_T> _arg;
    };
};

#endif
