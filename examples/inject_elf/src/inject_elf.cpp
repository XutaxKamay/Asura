#include "pch.h"

#include "xklib.h"
#include <unistd.h>

namespace error_code
{
    enum : int
    {
        INVALID_ARGS = -1,
        NO_ERROR     = 0
    };
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: ./inject_shellcode_game <pid> "
                     "<path/to/elf>"
                  << "\n";
        return error_code::INVALID_ARGS;
    }

    try
    {
        std::string game_pid(argv[1]);

        auto process = XKLib::Process(std::stoi(game_pid));
        auto mmap    = process.mmap();

        XKLib::Kokabiel kok(argv[2]);

        XKLib::Kokabiel::InjectionInfo injection_info;

        auto task = process.createTask(nullptr);

        std::cout << std::hex
                  << "Injecting with base stack: " << task.baseStack()
                  << "\n";

        kok.inject<XKLib::Process::TASK_STACK_SIZE,
                   XKLib::Kokabiel::arch::X86>(mmap,
                                               {},
                                               {},
                                               task,
                                               injection_info);

        task.run<true>();

        std::cout << std::dec << "Running with ID: " << task.id()
                  << std::hex
                  << " and with routine start: " << task.routineAddress()
                  << ", waiting to exit ... "
                  << "\n";

        task.wait();

        std::cout << "Cleaning up"
                  << "\n";

        kok.freeInjection(injection_info);

        task.freeStack();
    }
    catch (XKLib::Exception& e)
    {
        std::cerr << e.msg() << "\n";
        std::cerr << strerror(errno) << "\n";
    }

    return error_code::NO_ERROR;
}
