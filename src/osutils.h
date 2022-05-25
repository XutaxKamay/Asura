#ifndef XKLIB_OSUTILS_H
#define XKLIB_OSUTILS_H

#include "elf.h"
#include "exception.h"
#include "memoryutils.h"
#include "pe.h"
#include "process.h"

namespace XKLib
{
    class OSUtils
    {
      private:
        /* the M bool is to say if the Portable Executable is mapped */
        template <bool M>
        static auto FindFromParsedPE(const ptr_t data,
                                     const std::string& funcName,
                                     const auto baseAddress)
          -> module_sym_t
        {
            const auto dos_header = view_as<
              const PE::IMAGE_DOS_HEADER* const>(data);

            const auto nt_parent_headers = view_as<
              const PE::IMAGE_PARENT_NT_HEADERS* const>(
              view_as<std::uintptr_t>(dos_header) + dos_header->e_lfanew);

            switch (nt_parent_headers->FileHeader.Machine)
            {
                case PE::IMAGE_FILE_MACHINE_I386:
                {
                    return PE::find_exported_function<std::uint32_t, M>(
                      dos_header,
                      nt_parent_headers,
                      funcName,
                      baseAddress,
                      FindExportedFunctionRunTime<M>);
                }

                case PE::IMAGE_FILE_MACHINE_IA64:
                {
                    if constexpr (sizeof(std::uintptr_t)
                                  >= sizeof(std::uint64_t))
                    {
                        return PE::find_exported_function<std::uint64_t, M>(
                          dos_header,
                          nt_parent_headers,
                          funcName,
                          baseAddress,
                          FindExportedFunctionRunTime<M>);
                    }
                    else
                    {
                        XKLIB_EXCEPTION("Can't parse PE 64 bit files on "
                                        "32 bit");
                    }
                }

                case PE::IMAGE_FILE_MACHINE_AMD64:
                {
                    if constexpr (sizeof(std::uintptr_t)
                                  >= sizeof(std::uint64_t))
                    {
                        return PE::find_exported_function<std::uint64_t, M>(
                          dos_header,
                          nt_parent_headers,
                          funcName,
                          baseAddress,
                          FindExportedFunctionRunTime<M>);
                    }
                    else
                    {
                        XKLIB_EXCEPTION("Can't parse PE 64 bit files on "
                                        "32 bit");
                    }
                }

                default:
                {
                    XKLIB_EXCEPTION(
                      "Unknown PE Machine: "
                      + std::to_string(
                        nt_parent_headers->FileHeader.Machine));
                }
            }
        }

        template <bool M>
        static auto FindFromParsedELF(const ptr_t data,
                                      const std::string& funcName,
                                      const auto baseAddress)
          -> module_sym_t
        {
            const auto elf_parent_header = view_as<
              const ELF::Elf_Parent_Ehdr* const>(data);

            switch (elf_parent_header->e_ident[ELF::EI_CLASS])
            {
                case ELF::ELFCLASS32:
                {
                    return ELF::find_symbol<std::uint32_t, M>(
                      elf_parent_header,
                      funcName,
                      baseAddress);
                }

                case ELF::ELFCLASS64:
                {
                    if constexpr (sizeof(std::uintptr_t)
                                  >= sizeof(std::uint64_t))
                    {
                        return ELF::find_symbol<std::uint64_t, M>(
                          elf_parent_header,
                          funcName,
                          baseAddress);
                    }
                    else
                    {
                        XKLIB_EXCEPTION("Can't parse ELF 64 bit files on "
                                        "32 bit");
                    }
                }

                default:
                {
                    XKLIB_EXCEPTION(
                      "Unknown ELF Class: "
                      + std::to_string(
                        elf_parent_header->e_ident[ELF::EI_CLASS]));
                }
            }
        }

      public:
        /*  M is to say if we want to search from mapped module. */
        template <bool M = true>
        static auto FindExportedFunctionRunTime(
          const std::string& modName,
          const std::string& funcName) -> module_sym_t
        {
            auto process = Process::self();

            process.refreshModules();

            const auto& modules = process.modules();

            const auto found_module = std::find_if(
              modules.begin(),
              modules.end(),
              [&](const Process::Module& module)
              {
                  const auto mod_path = module.path();

                  if (mod_path.find(modName) != std::string::npos)
                  {
                      return true;
                  }

                  return false;
              });

            if (found_module == modules.end())
            {
                return { 0, 0 };
            }

            const auto test_magic_numbers_and_parse = [&](const auto data)
            {
                if (std::memcmp(data,
                                &ELF::MAGIC_NUMBER,
                                sizeof(ELF::MAGIC_NUMBER))
                    == 0)
                {
                    return FindFromParsedELF<M>(
                      data,
                      funcName,
                      found_module->baseAddress());
                }
                /* Wine compability */
                else if (std::memcmp(data,
                                     &PE::MAGIC_NUMBER,
                                     sizeof(PE::MAGIC_NUMBER))
                         == 0)
                {
                    return FindFromParsedPE<M>(
                      data,
                      funcName,
                      found_module->baseAddress());
                }
                else
                {
                    XKLIB_EXCEPTION("Could not find any compatible file "
                                    "format for "
                                    + found_module->path());
                }
            };

            if constexpr (not M)
            {
                std::ifstream file(found_module->path(),
                                   std::ios::binary | std::ios::in);

                if (not file.is_open())
                {
                    XKLIB_EXCEPTION("Couldn't open file "
                                    + found_module->path());
                    return {};
                }

                file.seekg(0, std::ios::end);
                const auto file_size = file.tellg();
                file.seekg(0, std::ios::beg);

                bytes_t file_bytes(file_size);
                file.read(view_as<char*>(file_bytes.data()), file_size);
                file.close();

                return test_magic_numbers_and_parse(file_bytes.data());
            }
            else
            {
                return test_magic_numbers_and_parse(
                  found_module->baseAddress());
            }
        }

        ///  static auto FindExportedFunctionRunTime(
        ///    const std::string& modName,
        ///    const std::string& funcName) -> T
        ///  {
        ///      return view_as<T>(
        ///        GetProcAddress(GetModuleHandleA(modName.c_str()),
        ///                       funcName.c_str()));
        ///  }

#ifdef WIN32
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
}

#endif
