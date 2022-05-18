#include "pch.h"

#include "xklib.h"

namespace error_code
{
    enum : int
    {
        INVALID_ARGS   = -2,
        CANT_OPEN_FILE = -1,
        NO_ERROR       = 0
    };
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: ./inject_shellcode_game <pid> "
                     "<path/to/shellcode>"
                  << "\n";
        return error_code::INVALID_ARGS;
    }

    try
    {
        std::string game_pid(argv[1]);

        auto process = XKLib::Process(std::stoi(game_pid));
        auto mmap    = process.mmap();

        std::ifstream shellcode_bin(argv[2],
                                    std::ios::binary | std::ios::in);

        if (not shellcode_bin.is_open())
        {
            std::cerr << "Could not open " << argv[2] << "\n";
            return error_code::CANT_OPEN_FILE;
        }

        shellcode_bin.seekg(0, std::ios::end);
        auto shellcode_size = shellcode_bin.tellg();
        shellcode_bin.seekg(0, std::ios::beg);

        const auto shellcode_start = mmap.allocArea(
          nullptr,
          shellcode_size,
          XKLib::MemoryArea::ProtectionFlags::RWX);

        std::cout << "Allocated memory for shellcode at: "
                  << reinterpret_cast<void*>(shellcode_start) << "\n";

        XKLib::bytes_t bytes;
        bytes.resize(shellcode_size);

        shellcode_bin.read(reinterpret_cast<char*>(bytes.data()),
                           bytes.size());

        mmap.write(shellcode_start, bytes);

        auto task = process.createTask(shellcode_start);

        std::cout << "Base stack: " << task.baseStack() << "\n";

        task.run<true>();

        std::cout << "Running with id: " << task.id() << "\n";

        task.wait();

        task.freeStack();

        mmap.freeArea(shellcode_start, shellcode_size);
    }
    catch (XKLib::Exception& e)
    {
        std::cerr << e.msg() << "\n";
        std::cerr << strerror(errno) << "\n";
    }

    return error_code::NO_ERROR;
}
