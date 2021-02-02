#ifndef OSUTILS_H
#define OSUTILS_H

#include "memoryutils.h"
#include "osutilsexception.h"

#ifndef WIN32
    #include <link.h>
#endif

namespace XLib
{
#ifndef WIN32
    typedef struct _IMAGE_OPTIONAL_HEADER32
    {
    } IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

    typedef struct _IMAGE_OPTIONAL_HEADER64
    {
    } IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

    typedef struct _IMAGE_FILE_HEADER
    {
    } IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;
#else
#endif

    typedef struct _PARENTNTHEADERS
    {
        uint32_t Signature;
        IMAGE_FILE_HEADER FileHeader;
    } PARENTNTHEADERS, *PPARENTNTHEADERS;

    typedef struct _NTHEADERS32
    {
        PARENTNTHEADERS Parent;
        IMAGE_OPTIONAL_HEADER32 OptionalHeader;
    } NTHEADERS32, *PNTHEADERS32;

    typedef struct _NTHEADERS64
    {
        PARENTNTHEADERS Parent;
        IMAGE_OPTIONAL_HEADER64 OptionalHeader;
    } NTHEADERS64, *PNTHEADERS64;

    class OSUtils
    {
      public:
#ifndef WIN32
        struct iterate_phdr_arg
        {
            std::vector<struct dl_phdr_info> infos;
        };
#endif

        static auto FindExportedFunctionRunTime(
          const std::string& modPath,
          const std::string& funcName) -> ptr_t;

#ifndef WIN32
        static auto FindDebugSymbol(const std::string& modName,
                                    const std::string& funcName) -> ptr_t;
#else
        static auto FindDebugSymbol(const std::string& funcName) -> ptr_t;
#endif
    };
};

#endif
