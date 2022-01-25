#ifndef KOKABIEL_H
#define KOKABIEL_H

#include "process.h"
#include "processmemoryarea.h"

namespace XKLib
{
    template <typename T>
    struct Elf_auxv_t
    {
        T a_type; /* Entry type */
        union
        {
            T a_val; /* Integer value */
        } a_un;
    };

    /**
     * @brief Manual maps a statically link ELF into a process.
     * TODO:
     * Support dynamic libs.
     */
    class Kokabiel
    {
        struct ready_segment_t
        {
            std::vector<byte_t> data;
            mapf_t flags;
            ptr_t start;
        };

      public:
        Kokabiel(const std::string& fileName);

        auto inject(Process& process,
                    const std::vector<std::string>& cmdLine,
                    const std::vector<std::string>& env) -> void;

      private:
        ELFIO::elfio _elf;
    };
};

#endif
