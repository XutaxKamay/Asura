#ifndef RSABLOCKS_H
#define RSABLOCKS_H

#include "exception.h"
#include "types.h"

#include <vendor/cryptopp/aes.h>
#include <vendor/cryptopp/modes.h>
#include <vendor/cryptopp/osrng.h>
#include <vendor/cryptopp/randpool.h>
#include <vendor/cryptopp/rdrand.h>
#include <vendor/cryptopp/rng.h>
#include <vendor/cryptopp/rsa.h>
#include <vendor/cryptopp/sha.h>
#include <vendor/cryptopp/zlib.h>

namespace XLib
{
    class RSABlocks
    {
      public:
        template <size_t rsa_key_size_T = 0x1000>
        static auto GenerateRSAPrivateKey()
        {
            using namespace CryptoPP;
            RSA::PrivateKey privateKey;

            AutoSeededRandomPool rng;
            privateKey.GenerateRandomWithKeySize(rng, rsa_key_size_T);

            return privateKey;
        }
    };

};

#endif
