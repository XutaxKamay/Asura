#include "process.h"
#include <unistd.h>

#ifdef WINDOWS
    #include <psapi.h>
#else
    #include <fcntl.h>
    #include <linux/limits.h>
#endif

using namespace XLib;

XLib::Process XLib::Process::find(const std::string& name)
{
#ifdef WINDOWS
    Process process;

    auto tool_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (!tool_handle)
    {
        return process;
    }

    PROCESSENTRY32 process_entry32;

    if (Process32First(tool_handle, &process_entry32))
    {
        auto match_process = [&name, &process_entry32, &process]()
        {
            if (name.find(process_entry32.szExeFile))
            {
                process = Process(process_entry32.th32ProcessID);
                return true;
            }

            return false;
        };

        if (!match_process())
        {
            while (Process32Next(tool_handle, &process_entry32))
            {
                if (match_process())
                {
                    break;
                }
            }
        }
    }

    CloseHandle(tool_handle);
#else
#endif

    if (process.id() == INVALID_PID)
    {
        XLIB_EXCEPTION("Couldn't find process: " + name);
    }

    return process;
}

auto XLib::Process::ProcessName(pid_t pid) -> std::string
{
    std::string result("unknown");

#ifndef WINDOWS
    result.reserve(PATH_MAX);

    if (readlink(
          std::string("/proc/" + std::to_string(pid) + "/exe").c_str(),
          result.data(),
          result.size())
        < 0)
    {
        throw XLIB_EXCEPTION("Could not read symlink.");
    }
#else
    result.reserve(MAX_PATH);

    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION
                                        | PROCESS_VM_READ,
                                      false,
                                      pid);

    if (!process_handle)
    {
        throw XLIB_EXCEPTION("Could not get process handle.");
    }

    if (GetModuleFileNameExA(process_handle,
                             nullptr,
                             result.data(),
                             result.size())
        <= 0)
    {
        throw XLIB_EXCEPTION("Could not read process path.");
    }

    CloseHandle(process_handle);
#endif

    return result;
}

auto Process::self() -> Process
{
#ifdef WINDOWS
    return Process(GetCurrentProcessId());
#else
    return Process(getpid());
#endif
}

Process::Process()
 : ProcessBase(INVALID_PID), _full_name("unknown"),
   _mmap(ProcessMemoryMap(*this))
{
}

Process::Process(pid_t pid)
 : ProcessBase(pid), _full_name(ProcessName(pid)),
   _mmap(ProcessMemoryMap(*this))
{
}

auto Process::tasks() -> tasks_t
{
    return Task::list(*this);
}

auto Process::mmap() -> ProcessMemoryMap
{
    _mmap.refresh();
    return _mmap;
}
