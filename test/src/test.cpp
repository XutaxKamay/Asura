#include "test.h"
#ifndef WINDOWS
    #include <sys/mman.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

#include <fstream>

using namespace XKLib;
using namespace CryptoPP;

#define ConsoleOutput(format) std::cout << "[XKLib] -> " << format

#ifndef WINDOWS
auto vfunc_hook(ptr_t thisptr) -> void
{
    std::cout << "hooked " << thisptr << std::endl;
}
#else
auto __fastcall vfunc_hook(ptr_t thisptr, ptr_t /* edx */) -> void
{
    std::cout << "hooked " << thisptr << std::endl;
}
#endif

auto rogue() -> void
{
    std::cout << "Hehe" << std::endl;
}

auto XKLib::Test::run() -> void
{
    ConsoleOutput("Starting test") << std::endl;

    std::string str("Life is a game, but you can not restart it."
                    "There might be no happy end.."
                    "But that's why people say to live your day"
                    " like it was the last,"
                    " because the pain "
                    "and the number of scars increase over time.");

    ConsoleOutput(str) << std::endl;

    WriteBuffer writeBuffer;

    auto strSize = view_as<size_t>(str.size());

    writeBuffer.addVar<type_array>(view_as<get_variable_t<type_array>>(
                                     str.data()),
                                   strSize);
    writeBuffer.addVar<type_array>(view_as<get_variable_t<type_array>>(
                                     str.data()),
                                   strSize);
    writeBuffer.addVar<type_array>(view_as<get_variable_t<type_array>>(
                                     str.data()),
                                   strSize);

    writeBuffer.addVar<type_8s>(1);
    writeBuffer.addVar<type_16s>(3);
    writeBuffer.addVar<type_32s>(3);
    writeBuffer.addVar<type_64s>(7);
    constexpr auto flValue = 42.42f;
    writeBuffer.addVar<type_float>(flValue);
    constexpr auto dlValue = 42.42;
    writeBuffer.addVar<type_double>(dlValue);

    auto bs = writeBuffer.toBytes();

    ConsoleOutput("Raw bytes:") << std::endl;

    for (auto&& b : bs)
    {
        std::cout << std::hex << view_as<int>(b);
    }

    std::cout << std::endl;

    ConsoleOutput("Generating RSA Key...") << std::endl;
    auto privateKey = RSABlocks::GenerateRSAPrivateKey();

    ConsoleOutput("Encrypting with RSA Key...") << std::endl;
    auto publicKey = RSA::PublicKey(privateKey);

    auto enc = EncryptRSABlocks(publicKey).encrypt(bs);

    ConsoleOutput("Raw encrypted bytes:") << std::endl;

    for (auto&& b : enc)
    {
        std::cout << std::hex << view_as<int>(b);
    }

    std::cout << std::endl;

    ConsoleOutput("Decrypting with RSA Key...") << std::endl;

    auto dec = DecryptRSABlocks(privateKey).decrypt(enc);

    ConsoleOutput("Raw decrypted bytes:") << std::endl;

    for (auto&& b : dec)
    {
        std::cout << std::hex << view_as<int>(b);
    }

    std::cout << std::endl;

    ReadBuffer readBuffer(dec.data(), dec.size());

    if (std::memcmp(readBuffer.data(),
                    writeBuffer.data(),
                    view_as<size_t>(writeBuffer.writeSize()))
        == 0)
    {
        ConsoleOutput("Passed decryption test") << std::endl;
    }
    else
    {
        ConsoleOutput("Passed decryption test failed") << std::endl;
        g_PassedTests = false;
    }

    auto ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize)) << std::endl;
    ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize)) << std::endl;
    ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize)) << std::endl;

    if (readBuffer.readVar<type_8s>() == 1
        && readBuffer.readVar<type_16s>() == 3
        && readBuffer.readVar<type_32s>() == 3
        && readBuffer.readVar<type_64s>() == 7
        && readBuffer.readVar<type_float>() == flValue
        && readBuffer.readVar<type_double>() == dlValue)
    {
        ConsoleOutput("Passed reading") << std::endl;
    }
    else
    {
        g_PassedTests = false;
    }

    if (g_PassedTests)
    {
        ConsoleOutput("Passed all tests") << std::endl;
    }
    else
    {
        ConsoleOutput("Failed test(s)") << std::endl;
    }

    using VAPI_t = VirtualTable<Test::API>;

    VAPI_t* api   = view_as<VAPI_t*>(&g_API);
    auto vptr_api = *view_as<ptr_t**>(api);

    api->callVFunc<0, void>(vptr_api);

    //     mapf_t flags;
    //
    //     auto pre_hook = [&flags](ptr_t funcPtr, ptr_t)
    //     {
    //         try
    //         {
    //             auto area = Process::self().mmap().search(funcPtr);
    //
    //             flags = area->protectionFlags().cachedValue();
    //
    //             area->protectionFlags() =
    //             MemoryArea::ProtectionFlags::RWX;
    //         }
    //         catch (Exception& me)
    //         {
    //             std::cout << std::endl;
    //
    //             std::cout << me.msg() << std::endl;
    //         }
    //     };
    //
    //     auto post_hook = [&flags](ptr_t funcPtr, ptr_t)
    //     {
    //         try
    //         {
    //             auto area = Process::self().mmap().search(funcPtr);
    //
    //             area->protectionFlags() = flags;
    //         }
    //         catch (Exception& me)
    //         {
    //             std::cout << std::endl;
    //
    //             std::cout << me.msg() << std::endl;
    //         }
    //     };

    hook_vfunc<0>(vptr_api, vfunc_hook /* , pre_hook, post_hook */);

    api->callVFunc<0, void>(vptr_api);

    ptr_t shellcode_address = nullptr;
    auto mmap               = Process::self().mmap();

    for (auto&& area : mmap.areas())
    {
        std::cout << std::hex << area->begin() << " -> " << area->name()
                  << std::endl;
    }

    try
    {
        shellcode_address = mmap.allocArea(nullptr,
                                           MemoryUtils::GetPageSize(),
                                           MemoryArea::ProtectionFlags::R);

        auto area = mmap.search(shellcode_address);

        ConsoleOutput("Allocated memory at ")
          << area->begin() << " for shellcode" << std::endl;

        auto write_test = mmap.allocArea(
#ifdef ENVIRONMENT64
          0x13370000ull,
#else
          0x13370000,
#endif
          MemoryUtils::GetPageSize(),
          MemoryArea::ProtectionFlags::RWX);

        area->protectionFlags() = MemoryArea::ProtectionFlags::RWX;

#ifndef WINDOWS
    #ifdef ENVIRONMENT64
        area->write({ 0xC7, 0x04, 0x25, 0x00, 0x00, 0x37, 0x13,
                      0x39, 0x05, 0x00, 0x00, 0x48, 0xC7, 0xC0,
                      0x3C, 0x00, 0x00, 0x00, 0x0F, 0x05 });
    #else
        area->write({ 0xC7,
                      0x05,
                      0x00,
                      0x00,
                      0x37,
                      0x13,
                      0x39,
                      0x05,
                      0x00,
                      0x00,
                      0xB8,
                      0x01,
                      0x00,
                      0x00,
                      0x00,
                      0xCD,
                      0x80 });
    #endif
#else
    #ifdef ENVIRONMENT64
        area->write({ 0xC7,
                      0x04,
                      0x25,
                      0x00,
                      0x00,
                      0x37,
                      0x13,
                      0x39,
                      0x05,
                      0x00,
                      0x00,
                      0xC3 });
    #else
        area->write({ 0xC7,
                      0x05,
                      0x00,
                      0x00,
                      0x37,
                      0x13,
                      0x39,
                      0x05,
                      0x00,
                      0x00,
                      0xC3 });
    #endif
#endif

        ConsoleOutput("write test: ")
          << *view_as<int*>(write_test) << std::endl;

        auto task = Process::self().createTask(shellcode_address);

        task.run();
        task.wait();

        area->protectionFlags() = MemoryArea::ProtectionFlags::R;

        ConsoleOutput("write test: ")
          << std::dec << *view_as<int*>(write_test) << std::endl;
    }
    catch (Exception& e)
    {
        std::cout << std::endl;

        std::cout << e.msg() << std::endl;
    }

    unsigned long long test_bits = 0b0111000011110000111100001111000011110000111100001111000011110000ull;

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    constexpr int mask_test_bits[] = { 4,  5,  6,  7,  12, 13, 14, 15,
                                       20, 21, 22, 23, 28, 29, 30, 31,
                                       36, 37, 38, 39, 44, 45, 46, 47,
                                       52, 53, 54, 55, 60, 61, 62 };

    bool tested_test_bits = true;

    for (int mask_test_bit : mask_test_bits)
    {
        if (!read_bit(&test_bits, mask_test_bit))
        {
            ConsoleOutput("wrong bit at pos: ")
              << mask_test_bit << std::endl;

            tested_test_bits = false;
            break;
        }
    }

    if (tested_test_bits)
    {
        ConsoleOutput("Passed read bits") << std::endl;
    }
    else
    {
        ConsoleOutput("Didn't pass read bits test") << std::endl;
    }

    write_bit<63, true>(&test_bits);

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    if (read_bit<63>(&test_bits))
    {
        ConsoleOutput("Passed write bits") << std::endl;
    }
    else
    {
        ConsoleOutput("Didn't pass write bits test") << std::endl;
    }

    auto net_write_buf = NetworkWriteBuffer(view_as<data_t>(&test_bits),
                                            sizeof(test_bits));

    try
    {
        net_write_buf.writeVar<type_64us>(0xFFFFFFFFFFFFFFFF);
    }
    catch (Exception& e)
    {
        ConsoleOutput(e.msg()) << std::endl;
    }

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    net_write_buf.pos();

    try
    {
        net_write_buf.writeVar<type_32us>(0);
    }
    catch (Exception& e)
    {
        ConsoleOutput(e.msg()) << std::endl;
    }

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    net_write_buf.pos();

    try
    {
        net_write_buf.writeVar<type_64us>(0xFFFFFFFFFFFFFFFF);
    }
    catch (Exception& e)
    {
        ConsoleOutput(e.msg()) << std::endl;
    }

    net_write_buf.pos(32);

    try
    {
        net_write_buf.writeVar<type_32us>(0);
    }
    catch (Exception& e)
    {
        ConsoleOutput(e.msg()) << std::endl;
    }

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    net_write_buf.pos();
    net_write_buf.writeVar<type_32us>(1337);
    net_write_buf.writeVar<type_32us>(1338);

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    auto net_read_buf = NetworkReadBuffer(view_as<data_t>(&test_bits),
                                          sizeof(test_bits));

    auto var1337 = net_read_buf.readVar<type_32us>();
    auto var1338 = net_read_buf.readVar<type_32us>();
    net_read_buf.pos();
    auto var1337_64 = net_read_buf.readVar<type_64us>();

    std::cout << std::dec;

    ConsoleOutput(var1337)
      << " " << var1338 << " " << var1337_64 << std::endl;

    auto intBits = bits_to_int<int>({ false, true, true, true });

    if (var1337_64 == test_bits && intBits == 14
        && int_to_bits(intBits)[0] == false
        && int_to_bits(intBits)[1] == true
        && int_to_bits(intBits)[2] == true
        && int_to_bits(intBits)[3] == true)
    {
        ConsoleOutput("Passed bits read test") << std::endl;
    }

    ConsoleOutput(intBits) << std::endl;

    std::vector<byte_t> random_bytes;

    for (int i = 0; i < 0x10000; i++)
    {
        static int add = 0;

        if (i % 5 == 0)
        {
            add++;

            if (add > 32)
            {
                add = 0;
            }
        }

        random_bytes.push_back(add + 63);
    }

    /*
    byte_t result;

    for (size_t i = 0; i < 0x1000; i++)
    {
        result = (rand() % 2) ? 64 : (rand() % 7 + 63);

        // result = rand() % 256;
        random_bytes.push_back(result);
    }
    */

    std::ofstream file("random_bytes.txt",
                       std::ios::binary | std::ios::out);

    file.write(view_as<char*>(random_bytes.data()),
               view_as<std::streamsize>(random_bytes.size()));
    file.close();

    ConsoleOutput("size of orginal: ")
      << random_bytes.size() << std::endl;

    {
        auto encoded = XKC<byte_t>::encode(random_bytes);

        ConsoleOutput("size of encoded: ") << encoded.size() << std::endl;

        auto decoded = XKC<byte_t>::decode(encoded);

        ConsoleOutput("size of decoded: ")
          << decoded.size() << " memcmp: "
          << std::memcmp(random_bytes.data(),
                         decoded.data(),
                         decoded.size())
          << std::endl;
    }

    try
    {
        std::vector<PatternByte::Value> pattern_bytes;

        for (auto&& value : random_bytes)
        {
            pattern_bytes.push_back(value);
        }

        pattern_bytes[5] = PatternByte::Value::UNKNOWN;

        PatternByte pattern(pattern_bytes);

        auto process = Process::self();

        Timer timer {};

        timer.start();
        pattern.scan(process);
        // PatternScanning::search(pattern, random_bytes, nullptr);
        timer.end();

        ConsoleOutput("scan took: ")
          << std::dec << timer.difference() << " nanoseconds "
          << "with: " <<
          [&process]()
        {
            auto mmap = process.mmap();

            size_t mmap_size = 0;

            for (auto&& area : mmap.areas())
            {
                mmap_size += area->size();
            }

            return std::to_string(view_as<double>(mmap_size) / 1000000.0);
        }()
          << " process memory in megabytes and "
          << pattern.values().size() << " of pattern size in bytes"
          << std::endl;

        if (pattern.matches().size() != 0)
        {
            ConsoleOutput("Found match(es):") << std::endl;

            for (auto&& match : pattern.matches())
            {
                std::cout
                  << "   " << match << " at pos: "
                  << view_as<ptr_t>(view_as<uintptr_t>(match) - 0x1337)
                  << std::endl;
            }
        }
        else
        {
            ConsoleOutput("Failed to find match(es)") << std::endl;
        }
    }
    catch (Exception& e)
    {
        ConsoleOutput(e.msg()) << std::endl;
    }

    class TestMember : Offset
    {
      public:
        TestMember()
        {
            first = new Something();
        }
        ~TestMember()
        {
            delete first;
        }

        class Something
        {
          public:
            int ok = 1337;
        };

        auto _first()
        {
            return *member_at<0x0, Something**>();
        }

        Something* first;
    };

    TestMember member;

    std::cout << member._first()->ok << std::endl;

    rogue();

    {
        static const std::string my_name = "xutaxkamay";
        auto encoded                     = XKC<byte_t>::encode(
          std::vector<byte_t>(my_name.begin(), my_name.end()));
        encoded.push_back('\0');

        ConsoleOutput(std::string(encoded.begin(), encoded.end()))
          << std::endl;
    }

    // std::getchar();
}

void XKLib::Test::API::func1()
{
    std::cout << "func1" << std::endl;
}

auto XKLib::Test::API::func2(const char* str, ...) -> std::vector<int>
{
    va_list parameterInfos;
    va_start(parameterInfos, str);

    char buffer[0x1000];
    sprintf(buffer, str, parameterInfos);

    std::cout << buffer << std::endl;

    va_end(parameterInfos);

    auto ints = view_as<int*>(buffer);

    std::vector<int> result;

    for (size_t i = 0; i < sizeof(buffer) / sizeof(int); i++)
    {
        result.push_back(ints[i]);
    }

    return result;
}
