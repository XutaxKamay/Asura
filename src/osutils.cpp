#include "osutils.h"

#ifndef WIN32
    #include <dlfcn.h>
    #include <fcntl.h>
    #include <link.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

using namespace XLib;

#ifdef WIN32
auto OSUtils::LoadOrFindExportedFunction(const std::string& modPath,
                                         const std::string& funcName)
  -> ptr_t
{
    return view_as<ptr_t>(
      GetProcAddress(LoadLibraryA(modName.c_str()), funcName.c_str()));
}
#else
auto OSUtils::LoadOrFindExportedFunction(const std::string& modName,
                                         const std::string& funcName)
  -> ptr_t
{
    return dlsym(dlopen(modName.c_str(), RTLD_LAZY), funcName.c_str());
}
#endif

#ifndef WIN32
static auto retrieveSymbolNames(struct dl_phdr_info* info,
                                size_t,
                                void* param) -> int
{
    auto arg = view_as<OSUtils::iterate_phdr_arg*>(param);

    if ((std::string(info->dlpi_name).find(arg->mod_name)
         == std::string::npos)
        || arg->addr != nullptr)
    {
        return 0;
    }

    auto fd = open(info->dlpi_name, O_RDONLY);

    if (fd < 0)
    {
        return 0;
    }

    struct stat st;

    if (fstat(fd, &st) < 0)
    {
        close(fd);
        return 0;
    }

    auto file_header = view_as<ElfW(Ehdr*)>(
      mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));

    close(fd);

    if (file_header->e_shoff == 0 || file_header->e_shstrndx == SHN_UNDEF)
    {
        munmap(file_header, st.st_size);
        return 0;
    }

    auto sections = view_as<ElfW(Shdr*)>(view_as<uintptr_t>(file_header)
                                         + file_header->e_shoff);

    auto section_count = file_header->e_shnum;

    auto section_names = view_as<uintptr_t>(file_header)
                         + sections[file_header->e_shstrndx].sh_offset;

    ElfW(Shdr*) section_sym_tab = nullptr;
    ElfW(Shdr*) section_str_tab = nullptr;

    for (auto section_num = 0; section_num < section_count; section_num++)
    {
        auto section      = &sections[section_num];
        auto section_name = view_as<const char*>(section_names
                                                 + section->sh_name);

        if (!strcmp(section_name, ".symtab"))
        {
            section_sym_tab = section;
        }
        else if (!strcmp(section_name, ".strtab"))
        {
            section_str_tab = section;
        }
    }

    if (section_str_tab == nullptr && section_sym_tab == nullptr)
    {
        munmap(file_header, st.st_size);
        return 0;
    }

    auto sym_tab = view_as<ElfW(Sym*)>(view_as<uintptr_t>(file_header)
                                       + section_sym_tab->sh_offset);

    auto str_tab = view_as<uintptr_t>(file_header)
                   + section_str_tab->sh_offset;

    auto sym_count = section_sym_tab->sh_size
                     / section_sym_tab->sh_entsize;

    for (decltype(sym_count) sym_index = 0; sym_index < sym_count;
         sym_index++)
    {
        auto sym = &sym_tab[sym_index];

        if (sym->st_shndx == SHN_UNDEF)
        {
            continue;
        }

        auto str_sym_name = std::string(
          view_as<const char*>(str_tab + sym->st_name));
        auto addr = info->dlpi_addr + sym->st_value;

        if (str_sym_name.find(arg->sym_name) != std::string::npos)
        {
            arg->addr = view_as<ptr_t>(addr);
            break;
        }
    }

    munmap(file_header, st.st_size);
    return 0;
}

auto OSUtils::FindDebugSymbol(const std::string& modName,
                              const std::string& funcName) -> ptr_t
{
    iterate_phdr_arg arg(modName, funcName);

    dl_iterate_phdr(retrieveSymbolNames, &arg);

    return arg.addr;
}
#else
auto OSUtils::FindDebugSymbol(const std::string&, const std::string&)
  -> ptr_t
{
    throw return nullptr;
}
#endif
