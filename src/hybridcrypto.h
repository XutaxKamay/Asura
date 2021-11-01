#ifndef HYBRIDCRYPT_H
#define HYBRIDCRYPT_H

#include <array>
#include <sstream>

#include <vendor/cryptopp/aes.h>
#include <vendor/cryptopp/modes.h>
#include <vendor/cryptopp/osrng.h>
#include <vendor/cryptopp/randpool.h>
#include <vendor/cryptopp/rdrand.h>
#include <vendor/cryptopp/rng.h>
#include <vendor/cryptopp/rsa.h>
#include <vendor/cryptopp/sha.h>
#include <vendor/cryptopp/zlib.h>

#include "types.h"

namespace XKLib
{
    template <size_t rsa_key_size_T = 0x1000>
    /**
     * @brief The HybridCrypt class
     * This class permits to have a AES and RSA combined powered with
     * CryptoPP. The rsa_key_size_T field permits to have a custom RSA key
     * size.
     */
    class HybridCrypt
    {
        /** HybridCrypt data /*/

        /**
         * @brief AESKeySize
         */
        static constexpr auto AESKeySize = CryptoPP::AES::MAX_KEYLENGTH;
        /**
         * @brief AESIVSize
         */
        static constexpr auto AESIVSize = CryptoPP::AES::BLOCKSIZE;
        /**
         * @brief The AESData_t struct
         */
        struct AESData_t
        {
            std::array<CryptoPP::byte, AESKeySize> key;
            std::array<CryptoPP::byte, AESIVSize> iv;
        };
    };

} // namespace XKLib

#endif // HYBRIDCRYPT_H
