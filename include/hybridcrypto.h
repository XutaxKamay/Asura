#ifndef HYBRIDCRYPT_H
#define HYBRIDCRYPT_H

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
    using namespace CryptoPP;
    using bytes = std::vector<byte>;

    template <int RSAKeySize = 0x1000>
    /**
     * @brief The HybridCrypt class
     * This class permits to have a AES and RSA combined powered with CryptoPP.
     * The RSAKeySize field permits to have a custom RSA key size.
     */
    class HybridCrypt
    {
        /** HybridCrypt data /*/

        /**
         * @brief AESKeySize
         */
        static constexpr auto AESKeySize = AES::MAX_KEYLENGTH;
        /**
         * @brief AESIVSize
         */
        static constexpr auto AESIVSize = AES::BLOCKSIZE;
        /**
         * @brief The AESData_t struct
         */
        struct AESData_t
        {
            byte iv[AESIVSize];
            byte key[AESKeySize];
            bool encrypted;
        };

       public:
        /**
         * @brief generateRSAKeys
         */
        auto generateRSAKeys() -> void;
        /**
         * @brief generateAESKey
         * @return
         */
        auto generateAESKey();
        /**
         * @brief decryptAESKey
         * @return
         */
        auto decryptAESKey();
        /**
         * @brief encryptAESKey
         * @return
         */
        auto encryptAESKey();
        /**
         * @brief encrypt
         * @param bs
         * @return
         */
        auto encrypt(bytes& bs);
        /**
         * @brief decrypt
         * @param bs
         * @return
         */
        auto decrypt(bytes& bs);
        /**
         * @brief privateKey
         * @return
         */
        auto privateKey() const;
        /**
         * @brief setPrivateKey
         * @param privateKey
         */
        auto setPrivateKey(const RSA::PrivateKey& privateKey) -> void;
        /**
         * @brief publicKey
         * @return
         */
        auto publicKey() const;
        /**
         * @brief setPublicKey
         * @param publicKey
         */
        auto setPublicKey(const RSA::PublicKey& publicKey) -> void;
        /**
         * @brief AESData
         * @return
         */
        auto AESData() const;
        /**
         * @brief setAESData
         * @param AESData_t
         */
        auto setAESData(const AESData_t& AESData_t) -> void;

       private:
        /**
         * @brief _privateKey
         */
        RSA::PrivateKey _privateKey;
        /**
         * @brief _publicKey
         */
        RSA::PublicKey _publicKey;
        /**
         * @brief _AESData
         */
        AESData_t _AESData;
    };

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::generateRSAKeys() -> void
    {
        AutoSeededRandomPool rng;

        _privateKey.GenerateRandomWithKeySize(rng, RSAKeySize);
        _publicKey = RSA::PublicKey(_privateKey);
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::generateAESKey()
    {
        AutoSeededRandomPool rnd;

        rnd.GenerateBlock(_AESData.key, AESKeySize);
        rnd.GenerateBlock(_AESData.iv, AESIVSize);

        _AESData.encrypted = false;

        return _AESData;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::decryptAESKey()
    {
        if (_AESData.encrypted)
        {
            Integer intAESData(_AESData, sizeof(AESData_t));

            RandomNumberGenerator rng;

            auto decryptAESKey = _privateKey.CalculateInverse(rng, intAESData);
            decryptAESKey.Encode(_AESData, decryptAESKey.MinEncodedSize());

            _AESData.encrypted = false;

            return true;
        }

        return false;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::encryptAESKey()
    {
        if (!_AESData.encrypted)
        {
            Integer intAESData(_AESData, sizeof(AESData_t));

            auto encryptAESKey = _privateKey.ApplyFunction(intAESData);
            encryptAESKey.Encode(_AESData, encryptAESKey.MinEncodedSize());

            _AESData.encrypted = true;

            return true;
        }

        return false;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::encrypt(bytes& bs)
    {
        if (!_AESData.encrypted)
        {
            CFB_Mode<AES>::Decryption cfbDecryption(_AESData.key,
                                                    AESKeySize,
                                                    _AESData.iv);

            cfbDecryption.ProcessData(bs.data(), bs.data(), bs.size());
            return true;
        }

        return false;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::decrypt(bytes& bs)
    {
        if (!_AESData.encrypted)
        {
            CFB_Mode<AES>::Encryption cfbEcryption(_AESData.key,
                                                   AESKeySize,
                                                   _AESData.iv);

            cfbEcryption.ProcessData(bs.data(), bs.data(), bs.size());
            return true;
        }

        return false;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::AESData() const
    {
        return _AESData;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::setAESData(const AESData_t& AESData_t) -> void
    {
        _AESData = AESData_t;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::publicKey() const
    {
        return _publicKey;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::setPublicKey(const RSA::PublicKey& publicKey)
        -> void
    {
        _publicKey = publicKey;
    }

    template <int RSAKeySize>
    auto HybridCrypt<RSAKeySize>::privateKey() const
    {
        return _privateKey;
    }

    template <int RSAKeySize>
    auto
    HybridCrypt<RSAKeySize>::setPrivateKey(const RSA::PrivateKey& privateKey)
        -> void
    {
        _privateKey = privateKey;
    }

}

#endif // HYBRIDCRYPT_H
