#ifndef PROCESSBASE_H
#define PROCESSBASE_H

#include "types.h"

namespace XKLib
{
    class Process;
    class Task;

    class ProcessBase
    {
        friend Process;
        friend Task;

      public:
        static auto self() -> ProcessBase;

      public:
        explicit ProcessBase(const process_id_t);

      public:
        auto id() const -> process_id_t;

      public:
        auto setPID(const process_id_t pid) -> void;

      private:
        process_id_t _pid;
    };
}

#endif
