#ifndef RSABLOCKS_H
#define RSABLOCKS_H

#include <sstream>

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/randpool.h>
#include <cryptopp/rdrand.h>
#include <cryptopp/rng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/sha.h>
#include <cryptopp/zlib.h>

#include "types.h"

namespace XLib
{
    using namespace CryptoPP;

    class RSABlocksException : std::exception
    {
      public:
        RSABlocksException(const std::string& msg);

        auto msg() -> std::string&;

      private:
        std::string _msg {};
    };

    class RSABlocks
    {
      public:
        template <size_t rsa_key_size_T = 0x1000>
        static auto GenerateRSAPrivateKey()
        {
            RSA::PrivateKey privateKey;

            AutoSeededRandomPool rng;
            privateKey.GenerateRandomWithKeySize(rng, rsa_key_size_T);

            return privateKey;
        }
    };

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
