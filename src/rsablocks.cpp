#include "rsablocks.h"
#include "readbuffer.h"
#include "writebuffer.h"

XLib::RSABlocksException::RSABlocksException(const std::string& msg)
 : _msg(msg)
{
}

auto XLib::RSABlocksException::msg() -> std::string&
{
    return _msg;
}

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
    auto original_size = view_as<g_v_t<type_64>>(bytes.size());

    /**
     * Write header
     */
    bytes.resize(bytes.size() + remainder + min_size);

    WriteBuffer(bytes.data(), false, bytes.size() - min_size, bytes.size())
      .addVar<type_64>(original_size);

    auto block_count_max = bytes.size() / min_size;

    for (decltype(block_count_max) block_count = 0;
         block_count < block_count_max;
         block_count++)
    {
        auto start = view_as<byte_t*>(view_as<uintptr_t>(bytes.data())
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

XLib::DecryptRSABlocks::DecryptRSABlocks(CryptoPP::Integer publicExponent,
                                         CryptoPP::Integer privateExponent,
                                         CryptoPP::Integer modulus)

{
    _private_key.Initialize(modulus, publicExponent, privateExponent);
}

XLib::DecryptRSABlocks::DecryptRSABlocks(RSA::PrivateKey privateKey)
 : _private_key(privateKey)
{
}

auto XLib::DecryptRSABlocks::decrypt(XLib::bytes_t bytes) -> bytes_t
{
    auto min_size = _private_key.GetModulus().MinEncodedSize();

    if (bytes.size() % min_size != 0)
    {
        throw RSABlocksException(std::string(CURRENT_CONTEXT)
                                 + "The size of the buffer must be a "
                                   "multiple of the key size\n");
    }

    auto block_count_max = bytes.size() / min_size;

    for (decltype(block_count_max) block_count = 0;
         block_count < block_count_max;
         block_count++)
    {
        auto start = view_as<byte_t*>(view_as<uintptr_t>(bytes.data())
                                      + block_count * min_size);

        Integer decrypt_block(start, min_size);

        AutoSeededRandomPool rng;
        auto decrypted_block = _private_key.CalculateInverse(
          rng,
          decrypt_block);

        decrypted_block.Encode(start, min_size);
    }

    auto original_size = ReadBuffer(bytes.data(),
                                    false,
                                    bytes.size() - min_size,
                                    bytes.size())
                           .readVar<type_64>();

    bytes.resize(original_size);

    return bytes;
}

auto& XLib::DecryptRSABlocks::privateKey()
{
    return _private_key;
}
