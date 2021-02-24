#include "encryptrsablocks.h"
#include "writebuffer.h"

using namespace CryptoPP;

XLib::EncryptRSABlocks::EncryptRSABlocks(CryptoPP::Integer publicExponent,
                                         CryptoPP::Integer modulus)
{
    _public_key.Initialize(modulus, publicExponent);
}

XLib::EncryptRSABlocks::EncryptRSABlocks(RSA::PublicKey publicKey)
 : _public_key(publicKey)
{
}

auto XLib::EncryptRSABlocks::encrypt(XLib::bytes_t bytes) -> bytes_t
{
    auto min_size      = _public_key.GetModulus().MinEncodedSize();
    auto remainder     = min_size - (bytes.size() % min_size);
    auto original_size = view_as<g_v_t<type_64s>>(bytes.size());

    /**
     * Write header
     */
    bytes.resize(bytes.size() + remainder + min_size);

    WriteBuffer(bytes.data(), bytes.size() - min_size, bytes.size())
      .addVar<type_64s>(original_size);

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

auto& XLib::EncryptRSABlocks::publicKey()
{
    return _public_key;
}
