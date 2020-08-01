#ifndef HYBRIDCRYPT_H
#define HYBRIDCRYPT_H

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
    using bytes = std::vector<byte>;

    template <safesize_t rsa_key_size_T = 0x1000>
    /**
     * @brief The HybridCrypt class
     * This class permits to have a AES and RSA combined powered with
     * CryptoPP. The rsa_key_size_T field permits to have a custom RSA key
     * size.
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
        };

      public:
        /**
         * @brief generateRSAKeys
         */
        auto generateRSAKeys(bool bRSAPublicKey = true) -> void;
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
         * @brief publicKeyFromPrivateKey
         * @return
         */
        auto publicKeyFromPrivateKey();
        /**
         * @brief AESData
         * @return AESData_t
         */
        auto AESData() const;
        /**
         * @brief setAESData
         * @param AESData_t
         * @return
         */
        auto setAESData(const AESData_t& AESData_t) -> void;

        /**
         * @brief debugKeys
         */
        auto debugKeys() -> void;

        auto isAESDataEncrypted() const;
        auto setIsAESDataEncrypted(bool isAESDataEncrypted) -> void;

        auto encodedAESDataSize() const -> int;
        auto setEncodedAESDataSize(int encodedAESDataSize) -> void;

        auto rng() const;
        auto setRng(const AutoSeededRandomPool& rng) -> void;

      private:
        /**
         * @brief _private_key
         */
        RSA::PrivateKey _private_key {};
        /**
         * @brief _public_key
         */
        RSA::PublicKey _public_key {};
        /**
         * @brief _aes_data
         */
        byte _aes_data[rsa_key_size_T];
        bool _is_aes_data_encrypted;
        int _encoded_aes_data_size;
        AutoSeededRandomPool _rng;
    };

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::generateRSAKeys(bool bRSAPublicKey)
      -> void
    {
        _private_key.GenerateRandomWithKeySize(_rng, rsa_key_size_T);

        if (bRSAPublicKey)
        {
            _public_key = RSA::PublicKey(_private_key);
        }
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::generateAESKey()
    {
        auto AESData = XLib::view_as<AESData_t*>(_aes_data);
        _rng.GenerateBlock(AESData->key, AESKeySize);
        _rng.GenerateBlock(AESData->iv, AESIVSize);

        _is_aes_data_encrypted = false;

        return _aes_data;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::decryptAESKey()
    {
        if (_is_aes_data_encrypted)
        {
            Integer intAESData(_aes_data, _encoded_aes_data_size);

            auto decryptAESKey = _private_key.CalculateInverse(_rng,
                                                               intAESData);

            decryptAESKey.Encode(_aes_data,
                                 decryptAESKey.MinEncodedSize());

            _is_aes_data_encrypted = false;

            return true;
        }

        return false;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::encryptAESKey()
    {
        if (!_is_aes_data_encrypted)
        {
            Integer intAESData(_aes_data, sizeof(AESData_t));

            auto encryptAESKey = _public_key.ApplyFunction(intAESData);

            _encoded_aes_data_size = encryptAESKey.MinEncodedSize();

            encryptAESKey.Encode(_aes_data, _encoded_aes_data_size);

            _is_aes_data_encrypted = true;

            return true;
        }

        return false;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::encrypt(bytes& bs)
    {
        if (!_is_aes_data_encrypted)
        {
            auto AESData = XLib::view_as<AESData_t*>(_aes_data);
            CFB_Mode<AES>::Decryption cfbDecryption(AESData->key,
                                                    AESKeySize,
                                                    AESData->iv);

            cfbDecryption.ProcessData(bs.data(), bs.data(), bs.size());
            return true;
        }

        return false;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::decrypt(bytes& bs)
    {
        if (!_is_aes_data_encrypted)
        {
            auto AESData = XLib::view_as<AESData_t*>(_aes_data);
            CFB_Mode<AES>::Encryption cfbEcryption(AESData->key,
                                                   AESKeySize,
                                                   AESData->iv);

            cfbEcryption.ProcessData(bs.data(), bs.data(), bs.size());
            return true;
        }

        return false;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::AESData() const
    {
        return _aes_data;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::setAESData(const AESData_t& AESData)
      -> void
    {
        std::memcpy(_aes_data, &AESData, sizeof(AESData_t));
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::publicKey() const
    {
        return _public_key;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::setPublicKey(
      const RSA::PublicKey& publicKey) -> void
    {
        _public_key = publicKey;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::publicKeyFromPrivateKey()
    {
        return RSA::PublicKey(_private_key);
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::privateKey() const
    {
        return _private_key;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::setPrivateKey(
      const RSA::PrivateKey& privateKey) -> void
    {
        _private_key = privateKey;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::debugKeys() -> void
    {
        const auto& nr = _public_key.GetModulus();
        const auto& er = _public_key.GetPublicExponent();

        const auto& np    = _private_key.GetModulus();
        const auto& epub  = _private_key.GetPublicExponent();
        const auto& epriv = _private_key.GetPrivateExponent();

        ConsoleOutput("RSA Public Key: Modulus\n") << nr << std::endl;

        ConsoleOutput("RSA Public Key: PublicExponent\n")
          << er << std::endl;

        ConsoleOutput("RSA Private Key: Modulus\n") << np << std::endl;

        ConsoleOutput("RSA Private Key: PublicExponent\n")
          << epub << std::endl;

        ConsoleOutput("RSA Private Key: PrivateExponent\n")
          << epriv << std::endl;

        ConsoleOutput("AES IV:\n");

        auto AESData = XLib::view_as<AESData_t*>(_aes_data);

        for (auto b : AESData->iv)
        {
            std::cout << std::hex << static_cast<int>(b);
        }

        std::cout << std::endl;

        ConsoleOutput("AES Key:\n");

        for (auto b : AESData->key)
        {
            std::cout << std::hex << static_cast<int>(b);
        }

        std::cout << std::endl;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::isAESDataEncrypted() const
    {
        return _is_aes_data_encrypted;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::setIsAESDataEncrypted(
      bool isAESDataEncrypted) -> void
    {
        _is_aes_data_encrypted = isAESDataEncrypted;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::encodedAESDataSize() const -> int
    {
        return _encoded_aes_data_size;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::setEncodedAESDataSize(
      int encodedAESDataSize) -> void
    {
        _encoded_aes_data_size = encodedAESDataSize;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::rng() const
    {
        return _rng;
    }

    template <safesize_t rsa_key_size_T>
    auto HybridCrypt<rsa_key_size_T>::setRng(
      const AutoSeededRandomPool& rng) -> void
    {
        _rng = rng;
    }

}

#endif // HYBRIDCRYPT_H
