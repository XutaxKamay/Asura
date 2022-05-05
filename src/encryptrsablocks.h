#ifndef ENCRYPTRSABLOCKS_H
#define ENCRYPTRSABLOCKS_H

#include "rsablocks.h"

namespace XKLib
{
    class EncryptRSABlocks
    {
      public:
        EncryptRSABlocks(const CryptoPP::Integer& publicExponent,
                         const CryptoPP::Integer& modulus);
        explicit EncryptRSABlocks(
          const CryptoPP::RSA::PublicKey& publicKey);

      public:
        auto encrypt(const bytes_t& bytes) const -> bytes_t;
        auto publicKey() const -> const auto&;

      public:
        auto publicKey() -> auto&;

      private:
        CryptoPP::RSA::PublicKey _public_key {};
    };
};

#endif
