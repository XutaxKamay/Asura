#include "pch.h"

#include "encryptrsablocks.h"
#include "writebuffer.h"

using namespace CryptoPP;

XKLib::EncryptRSABlocks::EncryptRSABlocks(const Integer& publicExponent,
                                          const Integer& modulus)
{
    _public_key.Initialize(modulus, publicExponent);
}

XKLib::EncryptRSABlocks::EncryptRSABlocks(const RSA::PublicKey& publicKey)
 : _public_key(publicKey)
{
}

auto XKLib::EncryptRSABlocks::encrypt(const bytes_t& bytes) const
  -> bytes_t
{
    auto result              = bytes;
    const auto min_size      = _public_key.GetModulus().MinEncodedSize();
    const auto remainder     = min_size - (result.size() % min_size);
    const auto original_size = view_as<g_v_t<type_64us>>(result.size());

    /**
     * Write header
     */
    result.resize(result.size() + remainder + min_size);

    WriteBuffer writeBuffer(result.data(),
                            result.size(),
                            result.size() - min_size);

    writeBuffer.addVar<type_64us>(original_size);

    AutoSeededRandomPool rng;
    /* randomize last bytes */
    for (std::size_t i = writeBuffer.writeSize(); i < result.size(); i++)
    {
        result[i] = view_as<byte_t>(
          (Integer(rng, 0, 255).ConvertToLong()));
    }

    const auto block_count_max = result.size() / min_size;

    for (std::size_t block_count = 0; block_count < block_count_max;
         block_count++)
    {
        const auto start = view_as<data_t>(
          view_as<std::uintptr_t>(result.data())
          + block_count * min_size);

        const Integer encrypt_block(start, min_size);

        const auto encrypted_block = _public_key.ApplyFunction(
          encrypt_block);

        encrypted_block.Encode(start, min_size);
    }

    return result;
}

auto XKLib::EncryptRSABlocks::publicKey() const -> const auto&
{
    return _public_key;
}

auto XKLib::EncryptRSABlocks::publicKey() -> auto&
{
    return _public_key;
}
