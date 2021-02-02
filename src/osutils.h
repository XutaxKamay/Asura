#ifndef OSUTILS_H
#define OSUTILS_H

#include "memoryutils.h"

namespace XLib
{
    class OSUtils
    {
      public:
#ifndef WIN32
        struct iterate_phdr_arg
        {
            iterate_phdr_arg(const std::string& modName,
                             const std::string& symName)
             : mod_name(modName), sym_name(symName)
            {
            }

            std::string mod_name;
            std::string sym_name;
            ptr_t addr = nullptr;
        };
#endif

        static auto LoadOrFindExportedFunction(const std::string& modPath,
                                               const std::string& funcName)
          -> ptr_t;

        static auto FindDebugSymbol(const std::string& modName,
                                    const std::string& funcName) -> ptr_t;
    };
};

#endif
