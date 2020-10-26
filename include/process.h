#ifndef PROCESS_H
#define PROCESS_H

#include "processmap.h"

namespace XLib
{
    class Process
    {
      public:
        Process() = default;
        Process(const std::string& fullName, pid_t pid);

        auto setFullName(const std::string& fullName) -> void;
        auto fullName() const;

        auto setPID(pid_t pid);
        auto pid();

      private:
        std::string _full_name {};
        pid_t _pid {};
    };
} // namespace XLib

#endif
