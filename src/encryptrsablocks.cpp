#include "encryptrsablocks.h"
#include "writebuffer.h"

#include <random>
#include <utility>

using namespace CryptoPP;

XKLib::EncryptRSABlocks::EncryptRSABlocks(
  const CryptoPP::Integer& publicExponent,
  const CryptoPP::Integer& modulus)
{
    _public_key.Initialize(modulus, publicExponent);
}

XKLib::EncryptRSABlocks::EncryptRSABlocks(RSA::PublicKey publicKey)
 : _public_key(std::move(publicKey))
{
}

auto XKLib::EncryptRSABlocks::encrypt(XKLib::bytes_t bytes) -> bytes_t
{
    auto min_size      = _public_key.GetModulus().MinEncodedSize();
    auto remainder     = min_size - (bytes.size() % min_size);
    auto original_size = view_as<g_v_t<type_64us>>(bytes.size());

    /**
     * Write header
     */
    bytes.resize(bytes.size() + remainder + min_size);

    WriteBuffer writeBuffer(bytes.data(),
                            bytes.size(),
                            bytes.size() - min_size);

    writeBuffer.addVar<type_64us>(original_size);

    AutoSeededRandomPool rng;
    /* randomize last bytes */
    for (size_t i = writeBuffer.writeSize(); i < bytes.size(); i++)
    {
        bytes[i] = view_as<byte_t>((Integer(rng, 0, 255).ConvertToLong()));
    }

    auto block_count_max = bytes.size() / min_size;

    for (decltype(block_count_max) block_count = 0;
         block_count < block_count_max;
         block_count++)
    {
        auto start = view_as<data_t>(view_as<uintptr_t>(bytes.data())
                                     + block_count * min_size);

        Integer encrypt_block(start, min_size);

        auto encrypted_block = _public_key.ApplyFunction(encrypt_block);

        encrypted_block.Encode(start, min_size);
    }

    return bytes;
}

auto XKLib::EncryptRSABlocks::publicKey() -> auto&
{
    return _public_key;
}
