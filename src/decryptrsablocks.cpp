#include "pch.h"

#include "decryptrsablocks.h"
#include "readbuffer.h"

using namespace CryptoPP;

XKLib::DecryptRSABlocks::DecryptRSABlocks(
  const CryptoPP::Integer& publicExponent,
  const CryptoPP::Integer& privateExponent,
  const CryptoPP::Integer& modulus)

{
    _private_key.Initialize(modulus, publicExponent, privateExponent);
}

XKLib::DecryptRSABlocks::DecryptRSABlocks(RSA::PrivateKey privateKey)
 : _private_key(std::move(privateKey))
{
}

auto XKLib::DecryptRSABlocks::decrypt(const bytes_t& bytes) const
  -> bytes_t
{
    auto result         = bytes;
    const auto min_size = _private_key.GetModulus().MinEncodedSize();

    if (result.size() % min_size != 0)
    {
        XKLIB_EXCEPTION("The size of the buffer must be a "
                        "multiple of the key size\n");
    }

    const auto block_count_max = result.size() / min_size;

    for (std::size_t block_count = 0; block_count < block_count_max;
         block_count++)
    {
        const auto start = view_as<data_t>(
          view_as<std::uintptr_t>(result.data())
          + block_count * min_size);

        const Integer decrypt_block(start, min_size);

        AutoSeededRandomPool rng;
        const auto decrypted_block = _private_key.CalculateInverse(
          rng,
          decrypt_block);

        decrypted_block.Encode(start, min_size);
    }

    const auto original_size = ReadBuffer(result.data(),
                                          result.size(),
                                          result.size() - min_size)
                                 .readVar<type_64us>();

    result.resize(original_size);

    return result;
}

auto XKLib::DecryptRSABlocks::privateKey() const -> const auto&
{
    return _private_key;
}

auto XKLib::DecryptRSABlocks::privateKey() -> auto&
{
    return _private_key;
}
