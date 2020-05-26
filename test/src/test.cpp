#include "test.h"
#ifndef WINDOWS
    #include <sys/mman.h>
#else
    #include <windows.h>
#endif

using namespace XLib;

XLib::Test::API XLib::g_API;
bool XLib::g_PassedTests = true;

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

    /* Let's do the tests */
    HybridCrypt<8192> hybridCrypt;

    ConsoleOutput("Generating AES keys") << std::endl;

    hybridCrypt.generateAESKey();

    ConsoleOutput("Generating RSA keys") << std::endl;

    hybridCrypt.generateRSAKeys();

    std::string str("Life is a game, but you can not restart it."
                    "There might be no happy end.."
                    "But that's why people say to live your day"
                    " like it was the last,"
                    " because the pain "
                    "and the number of scars increase over time.");

    ConsoleOutput(str) << std::endl;

    WriteBuffer<4096> writeBuffer;

    auto strSize = static_cast<safesize_t>(str.size());

    writeBuffer.addVar<type_array>(view_as<gvt<type_array>>(str.data()),
                                   strSize);
    writeBuffer.addVar<type_array>(view_as<gvt<type_array>>(str.data()),
                                   strSize);
    writeBuffer.addVar<type_array>(view_as<gvt<type_array>>(str.data()),
                                   strSize);

    writeBuffer.addVar<type_8>(1);
    writeBuffer.addVar<type_16>(3);
    writeBuffer.addVar<type_32>(3);
    writeBuffer.addVar<type_64>(7);
    constexpr auto flValue = 42.42f;
    writeBuffer.addVar<type_float>(flValue);
    constexpr auto dlValue = 42.42;
    writeBuffer.addVar<type_double>(dlValue);

    auto bs = writeBuffer.toBytes();

    ConsoleOutput("Raw bytes:") << std::endl;

    for (auto&& b : bs)
    {
        std::cout << std::hex << static_cast<int>(b);
    }

    std::cout << std::endl;

    /* Let's test the hybrid crypto */
    ConsoleOutput("Encrypting...") << std::endl << std::endl;
    hybridCrypt.encrypt(bs);

    hybridCrypt.debugKeys();
    ConsoleOutput("Encrypting AES Key...") << std::endl << std::endl;
    hybridCrypt.encryptAESKey();
    hybridCrypt.debugKeys();

    ConsoleOutput("Raw encrypted bytes:") << std::endl;
    for (auto&& b : bs)
    {
        std::cout << std::hex << static_cast<int>(b);
    }

    std::cout << std::endl;

    hybridCrypt.debugKeys();
    ConsoleOutput("Decrypting AES Key...") << std::endl << std::endl;
    hybridCrypt.decryptAESKey();
    hybridCrypt.debugKeys();
    ConsoleOutput("Decrypting...") << std::endl << std::endl;
    hybridCrypt.decrypt(bs);

    ConsoleOutput("Raw decrypted bytes:") << std::endl;
    for (auto&& b : bs)
    {
        std::cout << std::hex << static_cast<int>(b);
    }

    std::cout << std::endl;

    ReadBuffer readBuffer(bs.data());

    if (std::memcmp(readBuffer.pData(),
                    writeBuffer.pData(),
                    static_cast<size_t>(writeBuffer.writeSize()))
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

    if (readBuffer.readVar<type_8>() == 1
        && readBuffer.readVar<type_16>() == 3
        && readBuffer.readVar<type_32>() == 3
        && readBuffer.readVar<type_64>() == 7
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

    /* Virtual Functions Tests */

    /* CallVFunc< 0, void >( static_cast< ptr_t >( &g_API ) );
     *
     * auto ret1 = CallVFunc< 1, std::vector< int >, const char*, int >(
     *     static_cast< ptr_t >( &g_API ),
     *    "test",
     * 1337 );
     */

    using VAPI_t = VirtualTable<Test::API>;

    VAPI_t* api = static_cast<VAPI_t*>(&g_API);
    api->callVFunc<0, void>();

    /** TODO: finish MemoryUtils so we can use this */
    api->hook<0>(vfunc_hook);
    api->callVFunc<0, void>();

    std::cout << "Number of virtual funcs: " << api->countVFuncs()
              << std::endl;
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
