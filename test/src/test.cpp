#include "test.h"
#ifndef WINDOWS
    #include <sys/mman.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

using namespace XLib;

#define ConsoleOutput(format) std::cout << "[XLib] -> " << format

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

auto XLib::Test::run() -> void
{
    ConsoleOutput("Starting test") << std::endl;

    std::string str("Life is a game, but you can not restart it."
                    "There might be no happy end.."
                    "But that's why people say to live your day"
                    " like it was the last,"
                    " because the pain "
                    "and the number of scars increase over time.");

    ConsoleOutput(str) << std::endl;

    WriteBuffer<4096> writeBuffer;

    auto strSize = view_as<safesize_t>(str.size());

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

    ReadBuffer readBuffer(dec.data());

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

    VAPI_t* api = view_as<VAPI_t*>(&g_API);
    api->callVFunc<0, void>();

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
    //         catch (MemoryException& me)
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
    //         catch (MemoryException& me)
    //         {
    //             std::cout << std::endl;
    //
    //             std::cout << me.msg() << std::endl;
    //         }
    //     };

    api->hook<0>(vfunc_hook /* , pre_hook, post_hook */);

    api->callVFunc<0, void>();

    ConsoleOutput("Number of virtual funcs: ")
      << api->countVFuncs() << std::endl;

    ptr_t shellcode_address = nullptr;
    auto mmap               = Process::self().mmap();

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

        ConsoleOutput("write test: ")
          << *view_as<int*>(write_test) << std::endl;

        auto task = Process::self().createTask(shellcode_address);

        task.run();

        task.wait();

        area->protectionFlags() = MemoryArea::ProtectionFlags::R;

        ConsoleOutput("write test: ")
          << *view_as<int*>(write_test) << std::endl;
    }
    catch (MemoryException& me)
    {
        std::cout << std::endl;

        std::cout << me.msg() << std::endl;
    }

    unsigned long long test_bits = 0b0111000011110000111100001111000011110000111100001111000011110000ull;

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    constexpr int mask_test_bits[] = { 4,  5,  6,  7,  12, 13, 14, 15,
                                       20, 21, 22, 23, 28, 29, 30, 31,
                                       36, 37, 38, 39, 44, 45, 46, 47,
                                       52, 53, 54, 55, 60, 61, 62 };

    bool tested_test_bits = true;

    for (size_t i = 0; i < sizeof(mask_test_bits) / sizeof(int); i++)
    {
        if (!ReadBit(&test_bits, mask_test_bits[i]))
        {
            ConsoleOutput("wrong bit at pos: ")
              << mask_test_bits[i] << std::endl;
            ;
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

    WriteBit<63, true>(&test_bits);

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    if (ReadBit<63>(&test_bits))
    {
        ConsoleOutput("Passed write bits") << std::endl;
    }
    else
    {
        ConsoleOutput("Didn't pass write bits test") << std::endl;
    }

    auto net_write_buf = NetworkWriteBuffer(view_as<data_t>(&test_bits),
                                            true,
                                            0,
                                            sizeof(test_bits));

    try
    {
        net_write_buf.write<type_64us>(0xFFFFFFFFFFFFFFFF);
    }
    catch (BufferException& be)
    {
        ConsoleOutput(be.msg()) << std::endl;
    }

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    net_write_buf.pos(0);

    try
    {
        net_write_buf.write<type_32us>(0);
    }
    catch (BufferException& be)
    {
        ConsoleOutput(be.msg()) << std::endl;
    }

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    net_write_buf.pos(0);

    try
    {
        net_write_buf.write<type_64us>(0xFFFFFFFFFFFFFFFF);
    }
    catch (BufferException& be)
    {
        ConsoleOutput(be.msg()) << std::endl;
    }

    net_write_buf.pos(32);

    try
    {
        net_write_buf.write<type_32us>(0);
    }
    catch (BufferException& be)
    {
        ConsoleOutput(be.msg()) << std::endl;
    }

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    net_write_buf.pos(0);
    net_write_buf.write<type_32us>(1337);
    net_write_buf.write<type_32us>(1337);

    ConsoleOutput(std::bitset<64>(test_bits)) << std::endl;

    auto net_read_buf = NetworkReadBuffer(view_as<data_t>(&test_bits),
                                          true,
                                          0,
                                          sizeof(test_bits));

    auto var1337   = net_read_buf.read<type_32us>();
    auto var1337_2 = net_read_buf.read<type_32us>();
    net_read_buf.pos(0);
    auto var1337_64 = net_read_buf.read<type_64us>();

    std::cout << std::dec;

    ConsoleOutput(var1337)
      << " " << var1337_2 << " " << var1337_64 << std::endl;

    auto intBits = BitsToInt<int>({ false, true, true, true });

    if (var1337_64 == test_bits && intBits == 14
        && IntToBits(intBits)[0] == false && IntToBits(intBits)[1] == true
        && IntToBits(intBits)[2] == true && IntToBits(intBits)[3] == true)
    {
        ConsoleOutput("Passed bits read test") << std::endl;
    }

    ConsoleOutput(intBits) << std::endl;

    std::vector<byte_t> random_bytes;

    for (size_t i = 0; i < 0xFFF; i++)
    {
        random_bytes.push_back(rand() % 255);
    }

    PatternByte pattern({ random_bytes[5],
                          random_bytes[6],
                          random_bytes[7],
                          PatternByte::Value::type_t::UNKNOWN,
                          random_bytes[9] });
#ifdef ENVIRONMENT64
    PatternScanning::search(pattern,
                            random_bytes,
                            view_as<ptr_t>(0x1337ull));
#else
    PatternScanning::search(pattern, random_bytes, view_as<ptr_t>(0x1337));
#endif

    if (pattern.matches().size() != 0)
    {
        ConsoleOutput("Found match(es):") << std::endl;

        for (auto&& match : pattern.matches())
        {
            std::cout << "   " << match << " at pos: "
                      << view_as<ptr_t>(view_as<uintptr_t>(match)
                                        - 0x1337)
                      << std::endl;
        }
    }
    else
    {
        ConsoleOutput("Failed to find match(es)") << std::endl;
    }

    // std::getchar();
}

void XLib::Test::API::func1()
{
    std::cout << "func1" << std::endl;
}

std::vector<int> XLib::Test::API::func2(const char* str, ...)
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
