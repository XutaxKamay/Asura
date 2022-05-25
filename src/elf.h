#ifndef XKLIB_ELF_H
#define XKLIB_ELF_H

#include "exception.h"
#include "types.h"

namespace XKLib
{
    namespace ELF
    {
        constexpr inline std::uint32_t MAGIC_NUMBER = 0x464c457f;

        constexpr inline auto EI_NIDENT = 16;
        constexpr inline auto EI_CLASS  = 4;

        enum : std::uint8_t
        {
            ELFCLASSNONE,
            ELFCLASS32,
            ELFCLASS64
        };

        enum : std::uint32_t
        {
            SHT_NULL,
            SHT_PROGBITS,
            SHT_SYMTAB,
            SHT_STRTAB,
            SHT_RELA,
            SHT_HASH,
            SHT_DYNAMIC,
            SHT_NOTE,
            SHT_NOBITS,
            SHT_REL,
            SHT_SHLIB,
            SHT_DYNSYM,
            SHT_LOPROC,
            SHT_HIPROC,
            SHT_LOUSER,
            SHT_HIUSER
        };

        enum : std::uint32_t
        {
            PT_NULL,
            PT_LOAD,
            PT_DYNAMIC,
            PT_INTERP,
            PT_NOTE,
            PT_SHLIB,
            PT_PHDR,
            PT_LOPROC,
            PT_HIPROC,
            PT_GNU_STACK
        };

        enum : int
        {
            DT_NULL,
            DT_NEEDED,
            DT_PLTRELSZ,
            DT_PLTGOT,
            DT_HASH,
            DT_STRTAB,
            DT_SYMTAB,
            DT_RELA,
            DT_RELASZ,
            DT_RELAENT,
            DT_STRSZ,
            DT_SYMENT,
            DT_INIT,
            DT_FINI,
            DT_SONAME,
            DT_RPATH,
            DT_SYMBOLIC,
            DT_REL,
            DT_RELSZ,
            DT_RELENT,
            DT_PLTREL,
            DT_DEBUG,
            DT_TEXTREL,
            DT_JMPREL,
            DT_BIND_NOW,
            DT_RUNPATH,
            DT_LOPROC,
            DT_HIPROC,
            DT_GNU_HASH = 0x6ffffef5
        };

        template <typename T>
        concept IntType = std::is_same<std::uint32_t, T>::value or std::
          is_same<std::uint64_t, T>::value;

        struct Elf_Parent_Ehdr
        {
            std::uint8_t e_ident[EI_NIDENT];
            std::uint16_t e_type;
            std::uint16_t e_machine;
            std::uint32_t e_version;
        };

        template <IntType T>
        struct Elf_Ehdr : Elf_Parent_Ehdr
        {
            T e_entry;
            T e_phoff;
            T e_shoff;
            std::uint32_t e_flags;
            std::uint16_t e_ehsize;
            std::uint16_t e_phentsize;
            std::uint16_t e_phnum;
            std::uint16_t e_shentsize;
            std::uint16_t e_shnum;
            std::uint16_t e_shstrndx;
        };

        struct Elf32_Phdr
        {
            std::uint32_t p_type;
            std::uint32_t p_offset;
            std::uint32_t p_vaddr;
            std::uint32_t p_paddr;
            std::uint32_t p_filesz;
            std::uint32_t p_memsz;
            std::uint32_t p_flags;
            std::uint32_t p_align;
        };

        struct Elf64_Phdr
        {
            std::uint32_t p_type;
            std::uint32_t p_flags;
            std::uint64_t p_offset;
            std::uint64_t p_vaddr;
            std::uint64_t p_paddr;
            std::uint64_t p_filesz;
            std::uint64_t p_memsz;
            std::uint64_t p_align;
        };

        template <IntType T>
        struct Elf_Phdr :
         std::conditional<std::is_same<T, std::uint64_t>::value,
                          Elf64_Phdr,
                          Elf32_Phdr>::type
        {
        };

        template <IntType T>
        struct Elf_Shdr
        {
            std::uint32_t sh_name;
            std::uint32_t sh_type;
            T sh_flags;
            T sh_addr;
            T sh_offset;
            T sh_size;
            std::uint32_t sh_link;
            std::uint32_t sh_info;
            T sh_addralign;
            T sh_entsize;
        };

        struct Elf32_Sym
        {
            std::uint32_t st_name;
            std::uint32_t st_value;
            std::uint32_t st_size;
            std::uint8_t st_info;
            std::uint8_t st_other;
            std::uint16_t st_shndx;
        };

        struct Elf64_Sym
        {
            std::uint32_t st_name;
            std::uint8_t st_info;
            std::uint8_t st_other;
            std::uint16_t st_shndx;
            std::uint64_t st_value;
            std::uint64_t st_size;
        };

        template <IntType T>
        struct Elf_Sym :
         std::conditional<std::is_same<T, std::uint64_t>::value,
                          Elf64_Sym,
                          Elf32_Sym>::type
        {
        };

        template <IntType T>
        struct Elf_Rel
        {
            T r_offset;
            T r_info;
        };

        template <IntType T>
        struct Elf_Rela
        {
            T r_offset;
            T r_info;
            typename std::make_signed<T>::type r_addend;
        };

        template <IntType T>
        struct Elf_Dyn
        {
            typename std::make_signed<T>::type d_tag;

            union
            {
                T d_val;
                T d_ptr;
            } d_un;
        };

        template <IntType T>
        struct Elf_Nhdr
        {
            T n_namesz;
            T n_descsz;
            T n_type;
        };

        template <IntType T, bool M>
        auto find_symbol(const Elf_Parent_Ehdr* const elfParentHeader,
                         const std::string& funcName,
                         const auto baseAddress) -> module_sym_t
        {
            const auto elf_header = view_as<const Elf_Ehdr<T>* const>(
              elfParentHeader);

            const auto view_offset = [&]()
            {
                if constexpr (M)
                {
                    return elf_header->e_phoff;
                }
                else
                {
                    return elf_header->e_shoff;
                }
            }();

            if (not view_offset)
            {
                return { 0, 0 };
            }

            const auto view_count = [&]()
            {
                if constexpr (M)
                {
                    return elf_header->e_phnum;
                }
                else
                {
                    return elf_header->e_shnum;
                }
            }();

            /**
             * If mapped, then we use program header, otherwise section
             * header
             */

            using ph_or_sec_t = typename std::
              conditional<M, Elf_Phdr<T>, Elf_Shdr<T>>::type;

            const auto views = view_as<const ph_or_sec_t* const>(
              view_as<std::uintptr_t>(elf_header) + view_offset);

            for (std::uint16_t i = 0; i < view_count; i++)
            {
                const auto view = &views[i];

                if constexpr (M)
                {
                    const Elf_Sym<T>* symbol_table = nullptr;
                    std::size_t symbol_count       = 0;
                    std::uintptr_t string_table    = 0;
                    bool processed_pt_dynamic      = false;

                    switch (view->p_type)
                    {
                        case PT_DYNAMIC:
                        {
                            if (processed_pt_dynamic)
                            {
                                XKLIB_EXCEPTION("Can't contain more than"
                                                " one PT_DYNAMIC type");
                            }
                            else
                            {
                                processed_pt_dynamic = true;
                            }

                            for (auto dyn = view_as<const Elf_Dyn<T>*>(
                                   view_as<std::uintptr_t>(elf_header)
                                   + view->p_vaddr);
                                 dyn->d_tag != DT_NULL;
                                 dyn++)
                            {
                                switch (dyn->d_tag)
                                {
                                    case DT_SYMTAB:
                                    {
                                        if (symbol_table)
                                        {
                                            XKLIB_EXCEPTION(
                                              "Can't contain more "
                                              "than "
                                              "one symbol table in "
                                              "PT_DYNAMIC");
                                        }

                                        symbol_table = view_as<
                                          decltype(symbol_table)>(
                                          view_as<std::uintptr_t>(
                                            dyn->d_un.d_val));

                                        auto current_symbol = symbol_table;

                                        /**
                                         * HACK:
                                         * Usually symbol table is just
                                         * before string table, so that
                                         * ease our stuff.
                                         */
                                        while (view_as<std::uintptr_t>(
                                                 current_symbol)
                                               < string_table)
                                        {
                                            current_symbol++;
                                        }

                                        symbol_count = (current_symbol
                                                        - symbol_table);

                                        break;
                                    }
                                    case DT_STRTAB:
                                    {
                                        if (string_table)
                                        {
                                            XKLIB_EXCEPTION(
                                              "Can't contain more than "
                                              "one string table in "
                                              "PT_DYNAMIC");
                                        }

                                        string_table = dyn->d_un.d_val;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }

                    if (symbol_count and string_table and symbol_table)
                    {
                        for (
                          typename std::remove_const<decltype(symbol_count)>::type i = 0;
                          i < symbol_count;
                          i++)
                        {
                            const auto current_symbol = &symbol_table[i];

                            const auto symbol_name = view_as<
                              const char* const>(
                              string_table + current_symbol->st_name);

                            if (funcName == symbol_name)
                            {
                                return { view_as<std::uintptr_t>(
                                           baseAddress),
                                         view_as<std::uintptr_t>(
                                           current_symbol->st_value)
                                           + view_as<std::uintptr_t>(
                                             baseAddress) };
                            }
                        }

                        /* couldn't find symbol here ... */
                        break;
                    }
                }
                else
                {
                    switch (view->sh_type)
                    {
                        case SHT_DYNSYM:
                        case SHT_SYMTAB:
                        {
                            const auto string_table = view_as<
                                                        std::uintptr_t>(
                                                        elf_header)
                                                      + views[view->sh_link]
                                                          .sh_offset;

                            const auto symbol_count = view->sh_size /
                                                      /**
                                                       * Or
                                                       * view->sh_entsize
                                                       */
                                                      sizeof(Elf_Sym<T>);
                            ;

                            const auto symbol_table = view_as<
                              const Elf_Sym<T>* const>(
                              view_as<std::uintptr_t>(elf_header)
                              + view->sh_offset);

                            for (
                              typename std::remove_const<decltype(symbol_count)>::type i = 0;
                              i < symbol_count;
                              i++)
                            {
                                const auto current_symbol = &symbol_table[i];

                                const auto symbol_name = view_as<
                                  const char* const>(
                                  string_table + current_symbol->st_name);

                                if (funcName == symbol_name)
                                {
                                    return { view_as<std::uintptr_t>(
                                               baseAddress),
                                             view_as<std::uintptr_t>(
                                               current_symbol->st_value)
                                               + view_as<std::uintptr_t>(
                                                 baseAddress) };
                                }
                            }
                            break;
                        };
                    }
                }
            }

            return { 0, 0 };
        }

    };
};

#endif
