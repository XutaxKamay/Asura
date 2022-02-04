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
        explicit ProcessBase(process_id_t);

      public:
        auto setPID(process_id_t pid) -> void;
        auto id() -> process_id_t;

      private:
        process_id_t _pid {};
    };
};

#endif
