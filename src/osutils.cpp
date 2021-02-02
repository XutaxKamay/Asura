#include "osutils.h"

#include <cstring>
#include <fstream>

#ifndef WIN32
    #include <dlfcn.h>
    #include <fcntl.h>

    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#else
    #include <dbghelp.h>
#endif

using namespace XLib;

#ifndef WIN32
auto OSUtils::FindExportedFunctionRunTime(const std::string& modName,
                                          const std::string& funcName)
  -> ptr_t
{
    auto handle = dlopen(modName.c_str(), RTLD_NOLOAD);

    auto ret = dlsym(handle, funcName.c_str());

    if (handle)
    {
        dlclose(handle);
    }

    return ret;
}
#else
auto OSUtils::FindExportedFunctionRunTime(const std::string& modName,
                                          const std::string& funcName)
  -> ptr_t
{
    return view_as<ptr_t>(
      GetProcAddress(GetModuleHandleA(modName.c_str()), funcName.c_str()));
}
#endif

#ifndef WIN32
static auto retrieveSymbolNames(struct dl_phdr_info* info,
                                size_t,
                                void* param) -> int
{
    auto arg = view_as<OSUtils::iterate_phdr_arg*>(param);

    arg->infos.push_back(*info);

    return 0;
}

auto OSUtils::FindDebugSymbol(const std::string& modName,
                              const std::string& funcName) -> ptr_t
{
    iterate_phdr_arg arg;

    dl_iterate_phdr(retrieveSymbolNames, &arg);

    dl_phdr_info* found_info = nullptr;

    for (auto&& info : arg.infos)
    {
        if (std::string(info.dlpi_name).find(modName)
            != std::string::npos)
        {
            found_info = view_as<dl_phdr_info*>(
              alloca(sizeof(dl_phdr_info)));
            memcpy(found_info, &info, sizeof(dl_phdr_info));
            break;
        }
    }

    if (found_info == nullptr)
    {
        throw OSUtilsException(std::string(CURRENT_CONTEXT)
                               + "Couldn't find module in runtime.");
    }

    auto fd = open(found_info->dlpi_name, O_RDONLY);

    if (fd < 0)
    {
        throw OSUtilsException(std::string(CURRENT_CONTEXT)
                               + "Couldn't open module in runtime.");
    }

    struct stat st;

    if (fstat(fd, &st) < 0)
    {
        close(fd);
        throw OSUtilsException(std::string(CURRENT_CONTEXT)
                               + "Couldn't fstat module in runtime.");
    }

    auto file_header = view_as<ElfW(Ehdr*)>(
      mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));

    close(fd);

    if (file_header->e_shoff == 0 || file_header->e_shstrndx == SHN_UNDEF)
    {
        munmap(file_header, st.st_size);
        throw OSUtilsException(std::string(CURRENT_CONTEXT)
                               + "No symbols.");
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

        if (!std::strcmp(section_name, ".symtab"))
        {
            section_sym_tab = section;
        }
        else if (!std::strcmp(section_name, ".strtab"))
        {
            section_str_tab = section;
        }
    }

    if (section_str_tab == nullptr && section_sym_tab == nullptr)
    {
        munmap(file_header, st.st_size);
        throw OSUtilsException(std::string(CURRENT_CONTEXT)
                               + "No symbols.");
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
        auto addr = found_info->dlpi_addr + sym->st_value;

        if (str_sym_name.find(funcName) != std::string::npos)
        {
            return view_as<ptr_t>(addr);
        }
    }

    munmap(file_header, st.st_size);
    throw OSUtilsException(std::string(CURRENT_CONTEXT)
                           + "Cannot find function.");
    return nullptr;
}
#else
auto OSUtils::FindDebugSymbol(const std::string& funcName) -> ptr_t
{
    SYMBOL_INFO sym_info;

    sym_info.SizeOfStruct = sizeof(SYMBOL_INFO);
    sym_info.MaxNameLen   = funcName.size();

    if (SymFromName(GetCurrentProcess(), funcName.c_str(), &sym_info))
    {
        return view_as<ptr_t>(sym_info.Address);
    }

    throw OSUtilsException(std::string(CURRENT_CONTEXT)
                           + "Couldn't find symbol in runtime.");
}
#endif
