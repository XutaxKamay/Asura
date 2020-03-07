#include "hybridcrypto.h"

using namespace XLib;

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::generateRSAKeys()
{
    RandomNumberGenerator rng;
    _privateKey.GenerateRandomWithKeySize(rng, RSAKeySize);
    _publicKey = RSA::PublicKey(_privateKey);
}

template <int RSAKeySize>
typename HybridCrypt<RSAKeySize>::AESData_t
HybridCrypt<RSAKeySize>::generateAESKey()
{
    AutoSeededRandomPool rnd;

    rnd.GenerateBlock(_AESData.key, AESKeySize);
    rnd.GenerateBlock(_AESData.iv, AESIVSize);

    _AESData.encrypted = false;

    return _AESData;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::dencryptAESKey()
{
    if (_AESData.encrypted)
    {
        auto byteAESKey = static_cast<byte*>(&_AESData);

        Integer intAESData(byteAESKey, sizeof(AESData_t));

        RandomNumberGenerator rng;

        auto dencryptAESKey = _privateKey.CalculateInverse(rng, intAESData);

        dencryptAESKey.Encode(byteAESKey, dencryptAESKey.MinEncodedSize());

        _AESData.encrypted = false;

        return true;
    }

    return false;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::encryptAESKey()
{
    if (!_AESData.encrypted)
    {
        auto byteAESKey = static_cast<byte*>(&_AESData);

        Integer intAESData(byteAESKey, sizeof(AESData_t));

        auto encryptAESKey = _privateKey.ApplyFunction(intAESData);

        encryptAESKey.Encode(byteAESKey, encryptAESKey.MinEncodedSize());

        _AESData.encrypted = true;

        return true;
    }

    return false;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::encrypt(bytes& bs)
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
bool HybridCrypt<RSAKeySize>::decrypt(bytes& bs)
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
typename HybridCrypt<RSAKeySize>::AESData_t
HybridCrypt<RSAKeySize>::AESData() const
{
    return _AESData;
}

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::setAESData(const AESData_t& AESData_t)
{
    _AESData = AESData_t;
}

template <int RSAKeySize>
RSA::PublicKey HybridCrypt<RSAKeySize>::publicKey() const
{
    return _publicKey;
}

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::setPublicKey(const RSA::PublicKey& publicKey)
{
    _publicKey = publicKey;
}

template <int RSAKeySize>
RSA::PrivateKey HybridCrypt<RSAKeySize>::privateKey() const
{
    return _privateKey;
}

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::setPrivateKey(const RSA::PrivateKey& privateKey)
{
    _privateKey = privateKey;
}
