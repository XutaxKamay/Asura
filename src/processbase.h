#ifndef PROCESSBASE_H
#define PROCESSBASE_H

#include "types.h"

namespace XLib
{
    class Process;
    class Task;
    class ProcessBase
    {
        friend Process;
        friend Task;

        static auto self() -> ProcessBase;

      private:
        ProcessBase(pid_t);

      public:
        auto setPID(pid_t pid);
        auto id() -> pid_t;

      private:
        pid_t _pid {};
    };
};

#endif
