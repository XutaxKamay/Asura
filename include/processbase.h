#ifndef PROCESSBASE_H
#define PROCESSBASE_H

#include "types.h"

namespace XLib
{
    class Process;
    class ProcessBase
    {
        friend Process;

      private:
        ProcessBase(pid_t);

      public:
        auto setPID(pid_t pid);
        auto pid() -> pid_t;

      private:
        pid_t _pid {};
    };
};

#endif
