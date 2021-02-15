#ifndef DECRYPTRSABLOCKS_H
#define DECRYPTRSABLOCKS_H

#include "rsablocks.h"

namespace XLib
{
    class DecryptRSABlocks
    {
      public:
        DecryptRSABlocks(CryptoPP::Integer publicExponent,
                         CryptoPP::Integer privateExponent,
                         CryptoPP::Integer modulus);
        DecryptRSABlocks(CryptoPP::RSA::PrivateKey privateKey);

      public:
        auto& privateKey();
        auto decrypt(bytes_t bytes) -> bytes_t;

      public:
        CryptoPP::RSA::PrivateKey _private_key {};
    };
};

#endif
