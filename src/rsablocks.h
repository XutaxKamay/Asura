#ifndef RSABLOCKS_H
#define RSABLOCKS_H

#include "exception.h"
#include "types.h"

namespace XKLib
{
    class RSABlocks
    {
      public:
        template <std::size_t rsa_key_size_T = 0x1000>
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
