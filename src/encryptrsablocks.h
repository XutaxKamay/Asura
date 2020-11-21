#ifndef ENCRYPTRSABLOCKS_H
#define ENCRYPTRSABLOCKS_H

#include "rsablocks.h"

namespace XLib
{
    using namespace CryptoPP;

    class EncryptRSABlocks
    {
      public:
        EncryptRSABlocks(Integer publicExponent, Integer modulus);
        EncryptRSABlocks(RSA::PublicKey publicKey);

      public:
        auto& publicKey();
        auto encrypt(bytes_t bytes) -> bytes_t;

      private:
        RSA::PublicKey _public_key {};
    };
};

#endif
