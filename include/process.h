#ifndef PROCESS_H
#define PROCESS_H

#include "memoryutils.h"

namespace XLib
{
    class Process
    {
      public:
        Process() = default;
        Process(const maps_t& maps, const std::string fullName, pid_t pid);

        auto refresh() -> void;
        auto maps();

        auto setFullName(const std::string& fullName) -> void;
        auto fullName() const;

        auto setPID(pid_t pid);
        auto pid();

      private:
        maps_t _maps {};
        std::string _full_name {};
        pid_t _pid {};
    };
}

#endif
