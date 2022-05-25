#ifndef XKLIB_PE_H
#define XKLIB_PE_H

#include "exception.h"
#include "types.h"

namespace XKLib
{
    namespace PE
    {
        constexpr inline std::uint16_t MAGIC_NUMBER            = 0x5A4D;
        constexpr inline auto IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;
        constexpr inline auto IMAGE_DIRECTORY_ENTRY_EXPORT     = 0;
        constexpr inline auto IMAGE_SIZEOF_SHORT_NAME          = 8;
        constexpr inline auto IMAGE_FILE_MACHINE_I386          = 0x14c;
        constexpr inline auto IMAGE_FILE_MACHINE_IA64          = 0x200;
        constexpr inline auto IMAGE_FILE_MACHINE_AMD64         = 0x8664;

        /* For 32 bits programs, PE 32 bit only supported */
        template <typename T>
        concept IntType = std::is_same<std::uint32_t, T>::value or(
          std::is_same<std::uint64_t, T>::value);

        struct IMAGE_DOS_HEADER
        {
            std::uint16_t e_magic;
            std::uint16_t e_cblp;
            std::uint16_t e_cp;
            std::uint16_t e_crlc;
            std::uint16_t e_cparhdr;
            std::uint16_t e_minalloc;
            std::uint16_t e_maxalloc;
            std::uint16_t e_ss;
            std::uint16_t e_sp;
            std::uint16_t e_csum;
            std::uint16_t e_ip;
            std::uint16_t e_cs;
            std::uint16_t e_lfarlc;
            std::uint16_t e_ovno;
            std::uint16_t e_res[4];
            std::uint16_t e_oemid;
            std::uint16_t e_oeminfo;
            std::uint16_t e_res2[10];
            std::uint32_t e_lfanew;
        };

        struct IMAGE_FILE_HEADER
        {
            std::uint16_t Machine;
            std::uint16_t NumberOfSections;
            std::uint32_t TimeDateStamp;
            std::uint32_t PointerToSymbolTable;
            std::uint32_t NumberOfSymbols;
            std::uint16_t SizeOfOptionalHeader;
            std::uint16_t Characteristics;
        };

        struct IMAGE_DATA_DIRECTORY
        {
            std::uint32_t VirtualAddress;
            std::uint32_t Size;
        };

        template <IntType T>
        struct IMAGE_OPTIONAL_HEADER
        {
            std::uint16_t Magic;
            std::uint8_t MajorLinkerVersion;
            std::uint8_t MinorLinkerVersion;
            std::uint32_t SizeOfCode;
            std::uint32_t SizeOfInitializedData;
            std::uint32_t SizeOfUninitializedData;
            std::uint32_t AddressOfEntryPoint;
            std::uint32_t BaseOfCode;
            T ImageBase;
            std::uint32_t SectionAlignment;
            std::uint32_t FileAlignment;
            std::uint16_t MajorOperatingSystemVersion;
            std::uint16_t MinorOperatingSystemVersion;
            std::uint16_t MajorImageVersion;
            std::uint16_t MinorImageVersion;
            std::uint16_t MajorSubsystemVersion;
            std::uint16_t MinorSubsystemVersion;
            std::uint32_t Win32VersionValue;
            std::uint32_t SizeOfImage;
            std::uint32_t SizeOfHeaders;
            std::uint32_t CheckSum;
            std::uint16_t Subsystem;
            std::uint16_t DllCharacteristics;
            T SizeOfStackReserve;
            T SizeOfStackCommit;
            T SizeOfHeapReserve;
            T SizeOfHeapCommit;
            std::uint32_t LoaderFlags;
            std::uint32_t NumberOfrvaAndSizes;
            IMAGE_DATA_DIRECTORY
            DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
        };

        struct IMAGE_PARENT_NT_HEADERS
        {
            std::uint32_t Signature;
            IMAGE_FILE_HEADER FileHeader;
        };

        template <IntType T>
        struct IMAGE_NT_HEADERS : IMAGE_PARENT_NT_HEADERS
        {
            IMAGE_OPTIONAL_HEADER<T> OptionalHeader;
        };

        /* Workaround for offsetof */
        struct IMAGE_NT_HEADERS32
        {
            std::uint32_t Signature;
            IMAGE_FILE_HEADER FileHeader;
            IMAGE_OPTIONAL_HEADER<std::uint32_t> OptionalHeader;
        };

        struct IMAGE_NT_HEADERS64
        {
            std::uint32_t Signature;
            IMAGE_FILE_HEADER FileHeader;
            IMAGE_OPTIONAL_HEADER<std::uint64_t> OptionalHeader;
        };

        struct IMAGE_EXPORT_DIRECTORY
        {
            std::uint32_t Characteristics;
            std::uint32_t TimeDateStamp;
            std::uint16_t MajorVersion;
            std::uint16_t MinorVersion;
            std::uint32_t Name;
            std::uint32_t Base;
            std::uint32_t NumberOfFunctions;
            std::uint32_t NumberOfNames;
            std::uint32_t AddressOfFunctions;
            std::uint32_t AddressOfNames;
            std::uint32_t AddressOfNameOrdinals;
        };

        struct IMAGE_SECTION_HEADER
        {
            std::uint8_t Name[IMAGE_SIZEOF_SHORT_NAME];

            union
            {
                std::uint32_t PhysicalAddress;
                std::uint32_t VirtualSize;
            } Misc;

            std::uint32_t VirtualAddress;
            std::uint32_t SizeOfRawData;
            std::uint32_t PointerToRawData;
            std::uint32_t PointerToRelocations;
            std::uint32_t PointerToLinenumbers;
            std::uint16_t NumberOfRelocations;
            std::uint16_t NumberOfLinenumbers;
            std::uint32_t Characteristics;
        };

        template <IntType T>
        constexpr inline auto image_first_section(
          const IMAGE_NT_HEADERS<T>* const ntHeaders)
        {
            if constexpr (std::is_same<std::uint32_t, T>::value)
            {
                return view_as<IMAGE_SECTION_HEADER*>(
                  view_as<std::uintptr_t>(ntHeaders)
                  + offsetof(IMAGE_NT_HEADERS32, OptionalHeader)
                  + ntHeaders->FileHeader.SizeOfOptionalHeader);
            }

            return view_as<IMAGE_SECTION_HEADER*>(
              view_as<std::uintptr_t>(ntHeaders)
              + offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
              + ntHeaders->FileHeader.SizeOfOptionalHeader);
        }

        template <IntType T>
        auto rva_2_raw(
          const IMAGE_NT_HEADERS<T>* const ntHeaders,
          const std::uint32_t rva,
          std::optional<IMAGE_SECTION_HEADER**> sectionHeaderFound = std::
            nullopt) -> std::uint32_t
        {
            const auto section_header = image_first_section(ntHeaders);

            for (std::uint16_t i = 0;
                 i < ntHeaders->FileHeader.NumberOfSections;
                 i++)
            {
                const auto section = &section_header[i];

                if ((rva >= section->VirtualAddress)
                    and ((rva - section->VirtualAddress)
                         < section->Misc.VirtualSize))
                {
                    if (sectionHeaderFound != std::nullopt)
                    {
                        (*sectionHeaderFound.value()) = section;
                    }

                    return view_as<std::uint32_t>(
                             rva - section->VirtualAddress)
                           + section->PointerToRawData;
                }
            }

            return 0;
        }

        template <IntType T>
        auto raw_2_rva(
          const IMAGE_NT_HEADERS<T>* const ntHeaders,
          const std::uint32_t raw,
          std::optional<IMAGE_SECTION_HEADER**> sectionHeaderFound = std::
            nullopt) -> std::uint32_t
        {
            const auto section_header = image_first_section(ntHeaders);

            for (std::uint16_t i = 0;
                 i < ntHeaders->FileHeader.NumberOfSections;
                 i++)
            {
                const auto section = &section_header[i];

                if ((raw >= section->PointerToRawData)
                    and ((raw - section->PointerToRawData)
                         < section->SizeOfRawData))
                {
                    if (sectionHeaderFound != std::nullopt)
                    {
                        (*sectionHeaderFound.value()) = section;
                    }

                    return view_as<std::uint32_t>(
                             raw - section->PointerToRawData)
                           + section->VirtualAddress;
                }
            }

            return 0;
        }

        template <IntType T, bool M>
        auto find_exported_function(
          const IMAGE_DOS_HEADER* const dosHeader,
          const IMAGE_PARENT_NT_HEADERS* const ntParentHeaders,
          const std::string& funcName,
          const auto baseAddress,
          const std::function<auto(const std::string&, const std::string&)
                                ->module_sym_t> handleForwardedFunction)
          -> module_sym_t
        {
            const auto nt_headers = view_as<
              const IMAGE_NT_HEADERS<T>* const>(ntParentHeaders);

            const auto exp_dir_entry = &nt_headers->OptionalHeader
                                          .DataDirectory
                                            [IMAGE_DIRECTORY_ENTRY_EXPORT];

            const auto entry_rva  = exp_dir_entry->VirtualAddress;
            const auto entry_size = exp_dir_entry->Size;

            const auto get_va = [&](const auto rva)
            {
                /**
                 * If mapped, then we apply directly the relative virtual
                 * address
                 */
                if constexpr (M)
                {
                    return view_as<std::uintptr_t>(dosHeader) + rva;
                }
                /* else we need to convert file offset to RVA */
                else
                {
                    return view_as<std::uintptr_t>(dosHeader)
                           + rva_2_raw(nt_headers, rva);
                }
            };

            const auto export_directory = view_as<
              const IMAGE_EXPORT_DIRECTORY* const>(get_va(entry_rva));

            if (not export_directory)
            {
                return { 0, 0 };
            }

            const auto funcs = view_as<T*>(
              get_va(export_directory->AddressOfFunctions));

            const auto names = view_as<T*>(
              get_va(export_directory->AddressOfNames));

            const auto ordinals = view_as<T*>(
              get_va(export_directory->AddressOfNameOrdinals));

            /* dll.function */
            const auto do_forwarded_function = [&](const auto func_ptr)
            {
                const std::string& forward_func_name(
                  view_as<const char* const>(func_ptr));

                const auto dot_pos = forward_func_name.find('.');

                if (dot_pos != std::string::npos)
                {
                    const auto module_name = forward_func_name.substr(
                      0,
                      dot_pos + 1);

                    const auto real_func_name = forward_func_name.substr(
                      dot_pos + 1,
                      forward_func_name.size());

                    if (handleForwardedFunction == nullptr)
                    {
                        XKLIB_EXCEPTION("Pointer required for: "
                                        + forward_func_name);
                    }

                    return handleForwardedFunction(module_name,
                                                   real_func_name);
                }
                else
                {
                    XKLIB_EXCEPTION("Should never happen (but still "
                                    "happens?): "
                                    + forward_func_name);
                }
            };

            const auto process_func =
              [&](const auto func_rva) -> module_sym_t
            {
                /* is it forwarded ? */
                if (func_rva > entry_rva and func_rva < entry_size)
                {
                    return do_forwarded_function(get_va(func_rva));
                }
                else
                {
                    return { view_as<std::uintptr_t>(baseAddress),
                             view_as<std::uintptr_t>(
                               get_va(func_rva)
                               - view_as<std::uintptr_t>(dosHeader)
                               + view_as<std::uintptr_t>(baseAddress)) };
                }
            };

            if (std::all_of(funcName.begin(),
                            funcName.end(),
                            [](const std::uint8_t& c)
                            {
                                return std::isdigit(c);
                            }))
            {
                return process_func(
                  funcs[std::stoi(funcName) - export_directory->Base]);
            }
            else
            {
                for (std::uint16_t i = 0;
                     i < export_directory->NumberOfNames;
                     i++)
                {
                    const std::string func_name = view_as<
                      const char* const>(get_va(names[i]));

                    if (func_name.find(funcName) != std::string::npos)
                    {
                        continue;
                    }

                    return process_func(funcs[ordinals[i]]);
                }
            }

            return { 0, 0 };
        }
    }
}

#endif
