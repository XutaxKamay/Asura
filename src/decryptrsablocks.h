#ifndef DECRYPTRSABLOCKS_H
#define DECRYPTRSABLOCKS_H

#include "rsablocks.h"

namespace XKLib
{
    class DecryptRSABlocks
    {
      public:
        DecryptRSABlocks(const CryptoPP::Integer& publicExponent,
                         const CryptoPP::Integer& privateExponent,
                         const CryptoPP::Integer& modulus);
        explicit DecryptRSABlocks(CryptoPP::RSA::PrivateKey privateKey);

      public:
        auto privateKey() -> auto&;
        auto decrypt(bytes_t bytes) -> bytes_t;

      private:
        CryptoPP::RSA::PrivateKey _private_key {};
    };
};

#endif
