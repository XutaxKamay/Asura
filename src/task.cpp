#include "pch.h"

#include "exception.h"
#include "task.h"

using namespace XKLib;

auto Task::list(ProcessBase processBase) -> tasks_t
{
    tasks_t tasks;

#ifdef WINDOWS
    THREADENTRY32 te32;

    const auto thread_handle_snapshot = CreateToolhelp32Snapshot(
      TH32CS_SNAPTHREAD,
      0);

    if (thread_handle_snapshot == INVALID_HANDLE_VALUE)
        XKLIB_EXCEPTION("Could not get snapshot handle for "
                        "getting the task list");

    te32.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(thread_handle_snapshot, &te32))
    {
        do
        {
            if (te32.th32OwnerProcessID == processBase.id())
            {
                Task task(processBase, te32.th32ThreadID);

                tasks.push_back(task);
            }
        }
        while (Thread32Next(thread_handle_snapshot, &te32));
    }

    CloseHandle(thread_handle_snapshot);

#else
    std::filesystem::path filepath_tasks(
      "/proc/" + std::to_string(processBase.id()) + "/task/");

    for (const auto& task_id_path : filepath_tasks)
    {
        const auto task_id = task_id_path.generic_string();

        const Task task(processBase, std::stoi(task_id));

        tasks.push_back(task);
    }

    /**
     * Should always exists
     */
    std::ifstream file_childrens("/proc/"
                                 + std::to_string(processBase.id())
                                 + "/task/children");

    std::string line;
    while (std::getline(file_childrens, line))
    {
        if (line.empty())
        {
            break;
        }

        const Task task(processBase, std::stoi(line));

        tasks.push_back(task);
    }

    /**
     * The base process is also a task
     */
    tasks.push_back(Task(processBase, processBase.id()));
#endif

    return tasks;
}

Task::Task(ProcessBase processBase, thread_id_t id)
 : _process_base(processBase),
   _id(id)
{
}

Task::Task(ProcessBase processBase)
 : _process_base(processBase),
   _id(INVALID_ID)
{
}

auto Task::wait() const -> void
{
#ifdef WINDOWS
    const auto thread_handle = OpenThread(THREAD_QUERY_INFORMATION,
                                          false,
                                          view_as<DWORD>(_id));

    if (not thread_handle)
    {
        XKLIB_EXCEPTION("Don't have permissions to wait "
                        "for task termination");
    }

    WaitForSingleObject(thread_handle, INFINITE);

    CloseHandle(thread_handle);
#else
    while (::kill(_id, 0) != -1)
    {
        usleep(100);
    }
#endif
}

auto Task::kill() const -> void
{
#ifdef WINDOWS
    const auto thread_handle = OpenThread(THREAD_TERMINATE,
                                          false,
                                          view_as<DWORD>(_id));

    if (not thread_handle)
    {
        XKLIB_EXCEPTION("Don't have permissions to terminate "
                        "task");
    }

    if (not TerminateThread(thread_handle, view_as<DWORD>(EXIT_CODE)))
    {
        XKLIB_EXCEPTION("Could not terminate task");
    }

    CloseHandle(thread_handle);
#else
    const auto ret = ::kill(_id, SIGKILL);

    if (ret != 0)
    {
        XKLIB_EXCEPTION("Could not terminate task");
    }
#endif
}

auto Task::id() const -> const thread_id_t&
{
    return _id;
}
