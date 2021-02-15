#ifndef RSABLOCKS_H
#define RSABLOCKS_H

#include "rsablocksexceptions.h"

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/randpool.h>
#include <cryptopp/rdrand.h>
#include <cryptopp/rng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/sha.h>
#include <cryptopp/zlib.h>

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
