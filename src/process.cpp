#include "pch.h"

#include "patternscanning.h"
#include "process.h"
#include "processmemoryarea.h"
#include "types.h"

using namespace XKLib;

Process::Module::Module(ptr_t baseAddress,
                        const std::string& name,
                        const std::string& path)
 : _base_address(baseAddress),
   _name(name),
   _path(path)
{
}

auto Process::Module::baseAddress() const -> const ptr_t&
{
    return _base_address;
}

auto Process::Module::name() const -> const std::string&
{
    return _name;
}

auto Process::Module::path() const -> const std::string&
{
    return _path;
}

auto Process::Module::baseAddress() -> ptr_t&
{
    return _base_address;
}

auto Process::Module::name() -> std::string&
{
    return _name;
}

auto Process::Module::path() -> std::string&
{
    return _path;
}

auto XKLib::Process::find(const std::string& name) -> XKLib::Process
{
    Process process;

#ifdef WINDOWS
    const auto tool_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,
                                                      0);

    if (not tool_handle)
    {
        goto end;
    }

    PROCESSENTRY32 process_entry32;
    process_entry32.dwSize = sizeof(process_entry32);

    if (Process32First(tool_handle, &process_entry32))
    {
        do
        {
            const auto exe_file_name = std::string(
              process_entry32.szExeFile);

            if (exe_file_name.find(name) != std::string::npos)
            {
                process = Process(process_entry32.th32ProcessID);
                break;
            }
        }
        while (Process32Next(tool_handle, &process_entry32));
    }

    CloseHandle(tool_handle);

#else
    for (const auto& entry : std::filesystem::directory_iterator("/proc"))
    {
        if (entry.is_directory())
        {
            process_id_t pid;

            try
            {
                pid = std::stoi(entry.path().filename());
            }
            /* could be fs or any folder that is not a number */
            catch (std::invalid_argument&)
            {
                continue;
            }

            const auto [result, found] = Process::name(pid);

            if (not found)
            {
                continue;
            }

            if (result.find(name) != std::string::npos)
            {
                process = Process(pid);
                break;
            }
        }
    }
#endif

#ifdef WINDOWS
end:
#endif
    if (process.id() == INVALID_PID)
    {
        XKLIB_EXCEPTION("Couldn't find process: " + name);
    }

    return process;
}

auto XKLib::Process::name(const process_id_t pid)
  -> std::tuple<std::string, bool>
{
    std::string result;
    bool found;

#ifndef WINDOWS
    result.resize(PATH_MAX);

    const auto real_size = readlink(
      std::string("/proc/" + std::to_string(pid) + "/exe").c_str(),
      result.data(),
      result.size());

    if (real_size < 0)
    {
        /* could be a kernel thread */
        found = false;

        /* XKLIB_EXCEPTION("Could not read symlink."); */
    }
    else
    {
        result.resize(real_size);
        found = true;
    }
#else
    result.resize(MAX_PATH);

    const auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION
                                              | PROCESS_VM_READ,
                                            false,
                                            view_as<DWORD>(pid));

    if (not process_handle)
    {
        XKLIB_EXCEPTION("Could not get process handle.");
    }

    const auto real_size = GetModuleFileNameExA(process_handle,
                                                nullptr,
                                                result.data(),
                                                result.size());
    if (real_size <= 0)
    {
        found = false;
    }
    else
    {
        result.resize(real_size);
        found = true;
    }

    CloseHandle(process_handle);
#endif

    return { result, found };
}

auto Process::self() -> Process
{
#ifdef WINDOWS
    return Process(GetCurrentProcessId());
#else
    return Process(getpid());
#endif
}

Process::Process() : ProcessBase(INVALID_PID), _full_name("unknown")
{
}

Process::Process(const process_id_t pid)
 : ProcessBase(pid),
   _mmap(ProcessMemoryMap(*this))
{
    const auto [result, found] = name(pid);

    if (not found)
    {
        return;
    }

    _full_name = result;

    refreshModules();
}

auto Process::tasks() const -> tasks_t
{
    return Task::list(*this);
}

auto Process::mmap() const -> const ProcessMemoryMap&
{
    return _mmap;
}

auto Process::modules() const -> const std::list<Module>&
{
    return _modules;
}

auto Process::search(PatternByte& patternByte) const -> void
{
    PatternScanning::searchInProcess(patternByte, *this);
}

auto Process::mmap() -> ProcessMemoryMap&
{
    return _mmap;
}

auto Process::modules() -> std::list<Module>&
{
    return _modules;
}

auto Process::refreshModules() -> void
{
    _modules.clear();

    const auto& mmap  = this->mmap();
    const auto& areas = mmap.areas();

    std::for_each(
      areas.begin(),
      areas.end(),
      [&](const std::shared_ptr<ProcessMemoryArea>& area)
      {
          if (area->isDeniedByOS() or not area->isReadable())
          {
              return;
          }

          const auto area_name = area->name();

          const auto path = std::filesystem::path(area->name());

          if (std::filesystem::exists(path))
          {
              const auto base_name = path.filename();

              _modules.push_back(
                { area->begin<ptr_t>(), base_name, area->name() });
          }
      });

    _modules.unique(
      [](const Module& moduleA, const Module& moduleB)
      {
          return (moduleA.name() == moduleB.name());
      });
}
