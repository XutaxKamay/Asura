#ifndef DECRYPTRSABLOCKS_H
#define DECRYPTRSABLOCKS_H

#include "rsablocks.h"

namespace XLib
{
    using namespace CryptoPP;

    class DecryptRSABlocks
    {
      public:
        DecryptRSABlocks(Integer publicExponent,
                         Integer privateExponent,
                         Integer modulus);
        DecryptRSABlocks(RSA::PrivateKey privateKey);

      public:
        auto& privateKey();
        auto decrypt(bytes_t bytes) -> bytes_t;

      public:
        RSA::PrivateKey _private_key {};
    };
};

#endif
