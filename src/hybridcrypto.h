#ifndef HYBRIDCRYPT_H
#define HYBRIDCRYPT_H

#include "types.h"

namespace XKLib
{
    template <std::size_t rsa_key_size_T = 0x1000>
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
