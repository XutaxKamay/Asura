#ifndef XKLIB_TASK_H
#define XKLIB_TASK_H

#include "memoryutils.h"
#include "processbase.h"

namespace XKLib
{
    class Task;

    template <std::size_t>
    class RunnableTask;

    using tasks_t     = std::list<Task>;
    using thread_id_t = std::int64_t;

    class Task
    {
        template <std::size_t>
        friend class RunnableTask;

      public:
        static inline auto EXIT_CODE         = 0x1338;
        static inline thread_id_t INVALID_ID = -1;

      public:
        static auto list(ProcessBase processBase) -> tasks_t;

      public:
        Task(ProcessBase processBase, const thread_id_t thread_id_t);

      private:
        explicit Task(ProcessBase processBase);

      public:
        auto wait() const -> void;
        auto kill() const -> void;
        auto id() const -> const thread_id_t&;

      private:
        ProcessBase _process_base;
        thread_id_t _id;
    };
};

#endif
