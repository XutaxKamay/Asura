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
        auto decrypt(const bytes_t& bytes) const -> bytes_t;
        auto privateKey() const -> const auto&;

      private:
        auto privateKey() -> auto&;

      private:
        CryptoPP::RSA::PrivateKey _private_key {};
    };
};

#endif
