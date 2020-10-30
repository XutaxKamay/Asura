#ifndef TASK_H
#define TASK_H

#include "memoryutils.h"
#include "processbase.h"
#include <list>

namespace XLib
{
    class Task;
    using tasks_t = std::list<Task>;
    using tid_t   = int;

    class Task
    {
      public:
        static inline auto EXIT_CODE = 0x1338;
        static auto list(ProcessBase* processBase) -> tasks_t;

      public:
        Task(ProcessBase* processBase, tid_t id);

      public:
        auto wait() -> void;
        auto kill() -> void;
        auto id() -> tid_t;

      private:
        ProcessBase* _process_base;
        tid_t _id;
    };
};

#endif
