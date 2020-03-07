#include <xlib.h>

using namespace XLib;

int main()
{
    ConsoleOutput("Starting test") << std::endl;

    /* Let's do the tests */
    HybridCrypt hybridCrypt;

    ConsoleOutput("Generating RSA and AES keys") << std::endl;

    hybridCrypt.generateAESKey();
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

    writeBuffer.addVar<type_array>(
        reinterpret_cast<gvt<type_array>>(str.data()), strSize);
    writeBuffer.addVar<type_array>(
        reinterpret_cast<gvt<type_array>>(str.data()), strSize);
    writeBuffer.addVar<type_array>(
        reinterpret_cast<gvt<type_array>>(str.data()), strSize);

    auto bs = writeBuffer.toBytes();

    for (auto&& b : bs)
    {
        std::cout << b;
    }

    std::cout << std::endl;

    /* Let's test the hybrid crypto */
    hybridCrypt.encrypt(bs);

    for (auto&& b : bs)
    {
        std::cout << b;
    }

    std::cout << std::endl;

    hybridCrypt.decrypt(bs);

    for (auto&& b : bs)
    {
        std::cout << b;
    }
    
    std::cout << std::endl;

    ReadBuffer readBuffer(bs.data());

    auto ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize));
    ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize));
    ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize));
    ptr = readBuffer.readVar<type_array>(&strSize);
    ConsoleOutput(std::string(ptr, ptr + strSize));

    return 0;
}
