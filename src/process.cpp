#include "pch.h"

#include "patternscanning.h"
#include "process.h"

using namespace XKLib;

auto XKLib::Process::find(const std::string& name) -> XKLib::Process
{
    Process process;

#ifdef WINDOWS
    auto tool_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (!tool_handle)
    {
        goto end;
    }

    PROCESSENTRY32 process_entry32;
    process_entry32.dwSize = sizeof(process_entry32);

    if (Process32First(tool_handle, &process_entry32))
    {
        do
        {
            if (name.find(process_entry32.szExeFile) != std::string::npos)
            {
                process = Process(process_entry32.th32ProcessID);
                break;
            }
        }
        while (Process32Next(tool_handle, &process_entry32));
    }

    CloseHandle(tool_handle);

#else
    for (auto&& entry : std::filesystem::directory_iterator("/proc"))
    {
        if (entry.is_directory())
        {
            auto pid = view_as<pid_t>(std::stoi(entry.path().filename()));

            if (name.find(ProcessName(pid)) != std::string::npos)
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

auto XKLib::Process::ProcessName(pid_t pid) -> std::string
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
        XKLIB_EXCEPTION("Could not read symlink.");
    }
#else
    result.reserve(MAX_PATH);

    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION
                                        | PROCESS_VM_READ,
                                      false,
                                      view_as<DWORD>(pid));

    if (!process_handle)
    {
        XKLIB_EXCEPTION("Could not get process handle.");
    }

    if (GetModuleFileNameExA(process_handle,
                             nullptr,
                             result.data(),
                             result.size())
        <= 0)
    {
        XKLIB_EXCEPTION("Could not read process path.");
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

auto XKLib::Process::search(XKLib::PatternByte& patternByte) -> void
{
    PatternScanning::searchInProcess(patternByte, *this);
}
