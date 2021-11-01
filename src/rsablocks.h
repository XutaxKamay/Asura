#ifndef RSABLOCKS_H
#define RSABLOCKS_H

#include "exception.h"
#include "types.h"

#include "../../XLib/cryptopp/aes.h"
#include "../../XLib/cryptopp//modes.h"
#include "../../XLib/cryptopp//osrng.h"
#include "../../XLib/cryptopp//randpool.h"
#include "../../XLib/cryptopp//rdrand.h"
#include "../../XLib/cryptopp//rng.h"
#include "../../XLib/cryptopp//rsa.h"
#include "../../XLib/cryptopp//sha.h"
#include "../../XLib/cryptopp//zlib.h"

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
