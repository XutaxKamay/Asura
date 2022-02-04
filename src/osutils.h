#ifndef OSUTILS_H
#define OSUTILS_H

#include "exception.h"
#include "memoryutils.h"

namespace XKLib
{
    class OSUtils
    {
      public:
#ifndef WIN32
        template <typename T>
        static auto FindExportedFunctionRunTime(
          const std::string& /*modName*/,
          const std::string& /*funcName*/) -> T
        {
            /**
             * TODO:
             * Do it properly
             */
            XKLIB_EXCEPTION("Cannot find function.");
            return view_as<T>(0);
        }
#else
        template <typename T>
        static auto FindExportedFunctionRunTime(
          const std::string& modName,
          const std::string& funcName) -> T
        {
            return view_as<T>(
              GetProcAddress(GetModuleHandleA(modName.c_str()),
                             funcName.c_str()));
        }
#endif

#ifndef WIN32
        template <typename T>
        auto FindDebugSymbol(const std::string& /*modName*/,
                             const std::string& /*funcName*/) -> T
        {
            /**
             * TODO:
             * Do it properly.
             */
            XKLIB_EXCEPTION("Cannot find function.");
            return view_as<T>(0);
        }
#else
        template <typename T>
        static auto FindDebugSymbol(const std::string& funcName) -> T
        {
            SYMBOL_INFO sym_info;

            sym_info.SizeOfStruct = sizeof(SYMBOL_INFO);
            sym_info.MaxNameLen   = view_as<ULONG>(funcName.size());

            if (SymFromName(GetCurrentProcess(),
                            funcName.c_str(),
                            &sym_info))
            {
                return view_as<T>(sym_info.Address);
            }

            return view_as<T>(0);
        }
#endif
    };
};

#endif
