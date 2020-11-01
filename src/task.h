#ifndef TASK_H
#define TASK_H

#include "memoryutils.h"
#include "processbase.h"
#include <list>

namespace XLib
{
    class Task;
    template <size_t stack_size_T>
    class RunnableTask;

    using tasks_t = std::list<Task>;
    using tid_t   = int;

    class Task
    {
        template <size_t>
        friend class RunnableTask;

      public:
        static inline auto EXIT_CODE  = 0x1338;
        static inline auto INVALID_ID = -1;

      public:
        static auto list(ProcessBase processBase) -> tasks_t;

      public:
        Task(ProcessBase processBase, tid_t id);

      private:
        Task(ProcessBase processBase);

      public:
        auto wait() -> void;
        auto kill() -> void;
        auto id() -> tid_t&;

      private:
        ProcessBase _process_base;
        tid_t _id;
    };
};

#endif
