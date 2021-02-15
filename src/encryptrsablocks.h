#ifndef ENCRYPTRSABLOCKS_H
#define ENCRYPTRSABLOCKS_H

#include "rsablocks.h"

namespace XLib
{
    class EncryptRSABlocks
    {
      public:
        EncryptRSABlocks(CryptoPP::Integer publicExponent,
                         CryptoPP::Integer modulus);
        EncryptRSABlocks(CryptoPP::RSA::PublicKey publicKey);

      public:
        auto& publicKey();
        auto encrypt(bytes_t bytes) -> bytes_t;

      private:
        CryptoPP::RSA::PublicKey _public_key {};
    };
};

#endif
