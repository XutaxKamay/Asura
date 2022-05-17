#include "pch.h"

#include "patternscanning.h"
#include "process.h"
#include "types.h"
#include <stdexcept>

using namespace XKLib;

auto XKLib::Process::find(const std::string& name) -> XKLib::Process
{
    Process process;

#ifdef WINDOWS
    const auto tool_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,
                                                      0);

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

            const auto [result, found] = ProcessName(pid);

            if (!found)
            {
                continue;
            }

	    std::cout << result << "\n";

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

auto XKLib::Process::ProcessName(const process_id_t pid)
  -> std::tuple<std::string, bool>
{
    std::string result;
    bool found;

#ifndef WINDOWS
    result.resize(PATH_MAX);

    if (readlink(
          std::string("/proc/" + std::to_string(pid) + "/exe").c_str(),
          result.data(),
          result.size())
        < 0)
    {
        /* could be a kernel thread */
        found = false;

        /* XKLIB_EXCEPTION("Could not read symlink."); */
    }
    else
    {
        found = true;
    }
#else
    result.resize(MAX_PATH);

    const auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION
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
        found = false;
    }
    else
    {
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

Process::Process()
 : ProcessBase(INVALID_PID), _full_name("unknown"),
   _mmap(ProcessMemoryMap(*this))
{
}

Process::Process(const process_id_t pid)
 : ProcessBase(pid), _mmap(ProcessMemoryMap(*this))
{
    const auto [result, found] = ProcessName(pid);
    if (!found)
    {
        return;
    }

    _full_name = result;
}

auto Process::tasks() const -> tasks_t
{
    return Task::list(*this);
}

auto Process::mmap() const -> const ProcessMemoryMap&
{
    return _mmap;
}

auto Process::search(PatternByte& patternByte) const -> void
{
    PatternScanning::searchInProcess(patternByte, *this);
}

auto Process::mmap() -> ProcessMemoryMap&
{
    return _mmap;
}
