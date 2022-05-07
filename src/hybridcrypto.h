#ifndef HYBRIDCRYPT_H
#define HYBRIDCRYPT_H

#include "types.h"

namespace XKLib
{
    template <std::size_t N = 0x1000>
    class HybridCrypt
    {
        static constexpr auto AESKeySize = CryptoPP::AES::MAX_KEYLENGTH;
        static constexpr auto AESIVSize  = CryptoPP::AES::BLOCKSIZE;

        struct AESData_t
        {
            std::array<CryptoPP::byte, AESKeySize> key;
            std::array<CryptoPP::byte, AESIVSize> iv;
        };
    };

}

#endif
