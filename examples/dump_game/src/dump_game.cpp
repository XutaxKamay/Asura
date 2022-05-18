#include "pch.h"

#include "xklib.h"

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
    if (argc < 2)
    {
        std::cerr << "Missing pid"
                  << "\n";
        return error_code::INVALID_ARGS;
    }

    try
    {
        std::string game_pid(argv[1]);

        const auto process = XKLib::Process(std::stoi(game_pid));
        const auto& mmap   = process.mmap();

        const std::string dump_folder = "./dumps/" + game_pid + "/";

        if (!std::filesystem::create_directories(dump_folder))
        {
            throw XKLib::Exception("Couldn't create directory at "
                                   + dump_folder);
        }

        for (auto&& area : mmap.areas())
        {
            std::cout << std::hex << "[ 0x" << area->begin() << " - 0x"
                      << area->end() << " ]"
                      << " -> " << area->name() << "\n";

            if (area->isDeniedByOS())
            {
                continue;
            }

            std::stringstream ss;
            ss << std::hex << area->begin() << "_" << area->end();

            auto area_name = area->name();

            for (auto&& c : area_name)
            {
                if (c == '/')
                {
                    c = '_';
                }
            }

            const auto file_name = dump_folder + area_name + "_"
                                   + ss.str() + "_"
                                   + std::to_string(area->protectionFlags()
                                                      .cachedValue());

            std::ofstream dump_file(file_name,
                                    std::ios::binary | std::ios::out);

            if (!dump_file.is_open())
            {
                throw XKLib::Exception("Could not open file" + file_name);
            }

            const auto old_flags = area->protectionFlags().cachedValue();

            area->protectionFlags().change(
              old_flags | XKLib::MemoryArea::ProtectionFlags::READ);

            try
            {
                const auto read_area = area->read(area->size());

                area->protectionFlags().change(old_flags);

                dump_file.write(XKLib::view_as<const char*>(
                                  read_area.data()),
                                read_area.size());

                dump_file.close();
            }
            catch (XKLib::Exception& e)
            {
                std::cerr << e.msg() << "\n";
                std::cerr << strerror(errno) << "\n";
            }
        }
    }
    catch (XKLib::Exception& e)
    {
        std::cerr << e.msg() << "\n";
        std::cerr << strerror(errno) << "\n";
    }

    return error_code::NO_ERROR;
}
