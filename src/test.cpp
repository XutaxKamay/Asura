#include "pch.h"

#include "test.h"
#include <cstdarg>

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

    auto pid = syscall_extended(__NR_getppid);

    std::cout << pid << " " << getppid() << std::endl;

    std::string str("Life is a game, but you can not restart it."
                    "There might be no happy end.."
                    "But that's why people say to live your day"
                    " like it was the last,"
                    " because the pain "
                    "and the number of scars increase over time.");

    ConsoleOutput(str) << std::endl;

    WriteBuffer writeBuffer;

    auto strSize = view_as<std::size_t>(str.size());

    writeBuffer.addVar<type_array>(
      view_as<get_variable_t<type_array>>(str.data()),
      strSize);
    writeBuffer.addVar<type_array>(
      view_as<get_variable_t<type_array>>(str.data()),
      strSize);
    writeBuffer.addVar<type_array>(
      view_as<get_variable_t<type_array>>(str.data()),
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
                    view_as<std::size_t>(writeBuffer.writeSize()))
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
        and readBuffer.readVar<type_16s>() == 3
        and readBuffer.readVar<type_32s>() == 3
        and readBuffer.readVar<type_64s>() == 7
        and readBuffer.readVar<type_float>() == flValue
        and readBuffer.readVar<type_double>() == dlValue)
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
        std::cout << std::hex << "[ " << area->begin() << " - "
                  << area->end() << " ]"
                  << " -> " << area->name() << std::endl;
    }

    try
    {
        shellcode_address = mmap.allocArea(nullptr,
                                           MemoryUtils::GetPageSize(),
                                           MemoryArea::ProtectionFlags::R);

        mmap.refresh();

        std::cout << "shellcode normally at " << shellcode_address
                  << std::endl;

        auto area = mmap.search(shellcode_address);

        if (area == nullptr)
        {
            throw Exception("oof 1");
        }

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
        area->write({ 0xE9, 0x0B, 0x00, 0x00, 0x00, 0xD1, 0x5A, 0xC1,
                      0xD1, 0xF1, 0xED, 0xD1, 0x5A, 0xC1, 0xD1, 0xF1,
                      0x48, 0xB8, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C,
                      0x20, 0x57, 0x48, 0x8D, 0x4C, 0x24, 0xF2, 0x48,
                      0x89, 0x44, 0x24, 0xF2, 0xB8, 0x0A, 0x00, 0x00,
                      0x00, 0xC7, 0x44, 0x24, 0xFA, 0x6F, 0x72, 0x6C,
                      0x64, 0x66, 0x89, 0x44, 0x24, 0xFE, 0xB8, 0x01,
                      0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x01, 0x00,
                      0x00, 0x00, 0x48, 0x89, 0xCE, 0x48, 0xC7, 0xC2,
                      0x0E, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xC7, 0x04,
                      0x25, 0x00, 0x00, 0x37, 0x13, 0x39, 0x05, 0x00,
                      0x00, 0xB8, 0x3C, 0x00, 0x00, 0x00, 0x48, 0xC7,
                      0xC7, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xC3 });
    #else
        area->write(
          { 0xE9, 0x0B, 0x00, 0x00, 0x00, 0xD1, 0x5A, 0xC1, 0xD1, 0xF1,
            0xED, 0xD1, 0x5A, 0xC1, 0xD1, 0xF1, 0x56, 0xB8, 0x0A, 0x00,
            0x00, 0x00, 0x53, 0x83, 0xEC, 0x10, 0xC7, 0x44, 0x24, 0x02,
            0x48, 0x65, 0x6C, 0x6C, 0x8D, 0x74, 0x24, 0x02, 0xC7, 0x44,
            0x24, 0x06, 0x6F, 0x2C, 0x20, 0x57, 0xC7, 0x44, 0x24, 0x0A,
            0x6F, 0x72, 0x6C, 0x64, 0x66, 0x89, 0x44, 0x24, 0x0E, 0xB8,
            0x04, 0x00, 0x00, 0x00, 0xBB, 0x01, 0x00, 0x00, 0x00, 0x89,
            0xF1, 0xBA, 0x0E, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xC7, 0x05,
            0x00, 0x00, 0x37, 0x13, 0x39, 0x05, 0x00, 0x00, 0xB8, 0x01,
            0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x80,
            0x83, 0xC4, 0x10, 0x5B, 0x5E, 0xC3 });
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

        task.run<false>();
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
        if (not read_bit(&test_bits, mask_test_bit))
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

    if (var1337_64 == test_bits and intBits == 14
        and int_to_bits(intBits)[0] == false
        and int_to_bits(intBits)[1] == true
        and int_to_bits(intBits)[2] == true
        and int_to_bits(intBits)[3] == true)
    {
        ConsoleOutput("Passed bits read test") << std::endl;
    }

    ConsoleOutput(intBits) << std::endl;

    bytes_t random_bytes;
    constexpr auto size_of_random = 0x8000ull;

    for (std::size_t i = 0; i < size_of_random; i++)
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

    Timer timer {};

    auto aligned_memory = align_alloc<data_t>(size_of_random * 8,
                                              sizeof(SIMD::value_t));

    try
    {
        std::vector<PatternByte::Value> pattern_bytes;

        random_bytes.clear();

        const char characters_set[] = { 'X', 'K', 'A', 'M',
                                        'Y', 'U', 'T', 'Z' };

        const char characters_set2[] = {
            'X', 'K', 'A', 'M', 'Y', 'U', 'T'
        };

        for (std::size_t i = 0; i < size_of_random; i++)
        {
            random_bytes.push_back(view_as<byte_t>(
              characters_set[rand() % sizeof(characters_set)]));
        }

        for (std::size_t i = 0; i < size_of_random / 8; i++)
        {
            pattern_bytes.push_back(view_as<byte_t>(
              characters_set2[rand() % sizeof(characters_set2)]));
        }

        int count = 0;

        for (auto&& pb : pattern_bytes)
        {
            if ((rand() % 256) == 0)
            {
                count++;
            }

            if (count > 0)
            {
                pb.value = PatternByte::Value::UNKNOWN;
                count++;

                if (count >= 1 + (rand() % 64))
                {
                    count = 0;
                }
            }
        }

        (pattern_bytes.end() - 1)->value = 'X';

        PatternByte pattern(pattern_bytes);

        auto process = Process::self();

        for (std::size_t i = 0; i < 8; i++)
        {
            std::memcpy(&aligned_memory[size_of_random * i],
                        random_bytes.data(),
                        size_of_random - 1);
        }

        auto random_start = sizeof(SIMD::value_t)
                            + rand()
                                % ((size_of_random * 8)
                                   - pattern_bytes.size()
                                   - sizeof(SIMD::value_t));

        random_start = MemoryUtils::Align(random_start,
                                          sizeof(SIMD::value_t));

        const auto end = random_start + pattern_bytes.size();

        size_t i = 0;
        for (; random_start < end; random_start++)
        {
            aligned_memory[random_start] = view_as<byte_t>(
              pattern_bytes[i].value);
            i++;
        }

        timer.start();

        for (int i = 0; i < 21; i++)
        {
            ConsoleOutput("V1: ") << i + 1 << std::endl;

            PatternScanning::searchV1(pattern,
                                      aligned_memory,
                                      random_bytes.size() * 8,
                                      nullptr);
        }

        ConsoleOutput("size: ") << pattern.matches().size() << "\n";
        pattern.matches().resize(pattern.matches().size() - 20);

        timer.end();

        ConsoleOutput("v1 scan took: ")
          << std::dec << timer.difference() << " nanoseconds "
          << "with: "
          << (random_bytes.size() * 8) / MemoryUtils::GetPageSize()
          << " page count and " << pattern.bytes().size()
          << " of pattern size in bytes" << std::endl;

        timer.start();

        for (int i = 0; i < 21; i++)
        {
            ConsoleOutput("V2: ") << i + 1 << std::endl;

            PatternScanning::searchV2(pattern,
                                      aligned_memory,
                                      random_bytes.size() * 8,
                                      nullptr);
        }

        ConsoleOutput("size: ") << pattern.matches().size() << "\n";
        pattern.matches().resize(pattern.matches().size() - 20);

        timer.end();

        ConsoleOutput("v2 scan took: ")
          << std::dec << timer.difference() << " nanoseconds "
          << "with: "
          << (random_bytes.size() * 8) / MemoryUtils::GetPageSize()
          << " page count and " << pattern.bytes().size()
          << " of pattern size in bytes" << std::endl;

        timer.start();

        for (int i = 0; i < 21; i++)
        {
            ConsoleOutput("V3: ") << i + 1 << std::endl;

            PatternScanning::searchV3(pattern,
                                      aligned_memory,
                                      random_bytes.size() * 8,
                                      nullptr);
        }

        ConsoleOutput("size: ") << pattern.matches().size() << "\n";
        pattern.matches().resize(pattern.matches().size() - 20);

        timer.end();

        ConsoleOutput("v3 scan took: ")
          << std::dec << timer.difference() << " nanoseconds "
          << "with: "
          << (random_bytes.size() * 8) / MemoryUtils::GetPageSize()
          << " page count and " << pattern.bytes().size()
          << " of pattern size in bytes" << std::endl;

        timer.start();

        for (int i = 0; i < 21; i++)
        {
            ConsoleOutput("V4: ") << i + 1 << std::endl;

            PatternScanning::searchV4(pattern,
                                      aligned_memory,
                                      random_bytes.size() * 8,
                                      nullptr);
        }

        ConsoleOutput("size: ") << pattern.matches().size() << "\n";
        pattern.matches().resize(pattern.matches().size() - 20);

        timer.end();

        ConsoleOutput("v4 scan took: ")
          << std::dec << timer.difference() << " nanoseconds "
          << "with: "
          << (random_bytes.size() * 8) / MemoryUtils::GetPageSize()
          << " page count and " << pattern.bytes().size()
          << " of pattern size in bytes" << std::endl;

        timer.start();

        for (int i = 0; i < 21; i++)
        {
            ConsoleOutput("V1 aligned: ") << i + 1 << std::endl;

            PatternScanning::searchAlignedV1(pattern,
                                             aligned_memory,
                                             random_bytes.size() * 8,
                                             nullptr);
        }

        ConsoleOutput("size: ") << pattern.matches().size() << "\n";
        pattern.matches().resize(pattern.matches().size() - 20);

        timer.end();

        ConsoleOutput("v1 aligned scan took: ")
          << std::dec << timer.difference() << " nanoseconds "
          << "with: "
          << (random_bytes.size() * 8) / MemoryUtils::GetPageSize()
          << " page count and " << pattern.bytes().size()
          << " of pattern size in bytes" << std::endl;

        /*  static PatternByte pattern2({ 'T',
                                        'A',
                                        'T',
                                        'G',
                                        'T',
                                        'G',
                                        'T',
                                        PatternByte::Value::UNKNOWN,
                                        'T',
                                        'G',
                                        'T',
                                        'G' });

          __attribute__((aligned(64))) static byte_t yey[] = {
              'G', 'C', 'A', 'A', 'T', 'G', 'C', 'G', 'T', 'A', 'T', 'G',
              'T', 'A', 'T', 'G', 'T', 'G', 'T', 'A', 'T', 'G', 'T', 'G',
              'T', 'A', 'T', 'G', 'T', 'G', 'T', 'A', 'T', 'G', 'T', 'G',
              'T', 'A', 'T', 'G', 'T', 'G', 'T', 'R', 'T', 'G', 'T', 'G',
              'A', 'T', 'A', 'T', 'G', 'T', 'G', 'T', 'G', 'T', 'Z', 'G',
              'T', 'A', 'T', 'G', 'T', 'G', 'T', 'A', 'T', 'G', 'T', 'M',
              'G', 'A', 'C', 'C', 'T', 'A', 'T', 'A', 'T', 'G', 'T', 'A',
              'T', 'A', 'V', 'G', 'T', 'G', 'T', 'A', 'T', 'G', 'T', 'G',
              'T', 'A', 'T', 'G', 'T', 'C', 'T', 'A', 'T', 'A', 'T', 'G',
              'T', 'A', 'T', 'G', 'T', 'G', 'T', 'G', 'T', 'Z', 'T', 'A',
              'T', 'G', 'T', 'G', 'T', 'G', 'T', 'Z'
          };

          timer.start();
          PatternScanning::searchV4(pattern, yey, sizeof(yey), nullptr);
          timer.end();

          ConsoleOutput("v4 scan took: ")
            << std::dec << timer.difference() << " nanoseconds "
            << "with: "
            << (random_bytes.size() * 8) / MemoryUtils::GetPageSize()
            << " page count and " << pattern.bytes().size()
            << " of pattern size in bytes" << std::endl;
            */

        if (pattern.matches().size() != 0)
        {
            ConsoleOutput("Found match(es):") << std::endl;

            for (auto&& match : pattern.matches())
            {
                std::cout << "   " << match << " at pos: "
                          << view_as<ptr_t>(view_as<std::uintptr_t>(match)
                                            - 0x1337)
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

    align_free(aligned_memory);

    class TestMember : public Offset
    {
      public:
        TestMember()
        {
            _first = new Something();
        }

        ~TestMember()
        {
            delete _first;
        }

        void call_me(int something)
        {
            std::cout << "hehe" << something << std::endl;
        }

        class Something
        {
          public:
            int ok = 1337;
        };

        auto first()
        {
            return *member<0x0, Something**>();
        }

        Something* _first;
    };

    TestMember member;
    auto method_ptr = &TestMember::call_me;
    member.call<void, int>(*view_as<ptr_t*>(&method_ptr), 3);

    std::cout << member.first()->ok << std::endl;

    rogue();

    try
    {
        const auto [self_proc_name, result] = Process::name(
          Process::self().id());

        const auto [mod_addr, symbol_addr] = OSUtils::
          FindExportedFunctionRunTime<true>(self_proc_name, "syscall");

        std::cout << std::hex << mod_addr << " " << symbol_addr
                  << std::endl;

        const auto [mod_addr2, symbol_addr2] = OSUtils::
          FindExportedFunctionRunTime<false>(self_proc_name, "syscall");

        std::cout << std::hex << mod_addr2 << " " << symbol_addr2
                  << std::endl;
    }
    catch (Exception& e)
    {
        std::cout << e.msg() << std::endl;
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

    for (std::size_t i = 0; i < sizeof(buffer) / sizeof(int); i++)
    {
        result.push_back(ints[i]);
    }

    return result;
}
