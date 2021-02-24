#include "decryptrsablocks.h"
#include "readbuffer.h"

using namespace CryptoPP;

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
        throw XLIB_EXCEPTION("The size of the buffer must be a "
                             "multiple of the key size\n");
    }

    auto block_count_max = bytes.size() / min_size;

    for (decltype(block_count_max) block_count = 0;
         block_count < block_count_max;
         block_count++)
    {
        auto start = view_as<data_t>(view_as<uintptr_t>(bytes.data())
                                     + block_count * min_size);

        Integer decrypt_block(start, min_size);

        AutoSeededRandomPool rng;
        auto decrypted_block = _private_key.CalculateInverse(
          rng,
          decrypt_block);

        decrypted_block.Encode(start, min_size);
    }

    auto original_size = ReadBuffer(bytes.data(),
                                    bytes.size(),
                                    bytes.size() - min_size)
                           .readVar<type_64s>();

    bytes.resize(original_size);

    return bytes;
}

auto& XLib::DecryptRSABlocks::privateKey()
{
    return _private_key;
}
