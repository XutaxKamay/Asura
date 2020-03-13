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
    using bytes = std::vector< byte >;

    template < int RSAKeySize = 0x1000 >
    /**
     * @brief The HybridCrypt class
     * This class permits to have a AES and RSA combined powered with
     * CryptoPP. The RSAKeySize field permits to have a custom RSA key
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
        auto encrypt( bytes& bs );
        /**
         * @brief decrypt
         * @param bs
         * @return
         */
        auto decrypt( bytes& bs );
        /**
         * @brief privateKey
         * @return
         */
        auto privateKey() const;
        /**
         * @brief setPrivateKey
         * @param privateKey
         */
        auto setPrivateKey( const RSA::PrivateKey& privateKey ) -> void;
        /**
         * @brief publicKey
         * @return
         */
        auto publicKey() const;
        /**
         * @brief setPublicKey
         * @param publicKey
         */
        auto setPublicKey( const RSA::PublicKey& publicKey ) -> void;
        /**
         * @brief AESData
         * @return
         */
        auto AESData() const;
        /**
         * @brief setAESData
         * @param AESData_t
         */
        auto setAESData( const AESData_t& AESData_t ) -> void;

        /**
         * @brief debugKeys
         */
        auto debugKeys() -> void;

        auto isAESDataEncrypted() const;
        auto setIsAESDataEncrypted( bool isAESDataEncrypted ) -> void;

        auto encodedAESDataSize() const -> int;
        auto setEncodedAESDataSize( int encodedAESDataSize ) -> void;

        auto rng() const;
        auto setRng( const AutoSeededRandomPool& rng ) -> void;

      private:
        /**
         * @brief _privateKey
         */
        RSA::PrivateKey _privateKey {};
        /**
         * @brief _publicKey
         */
        RSA::PublicKey _publicKey {};
        /**
         * @brief _AESData
         */
        byte _AESData[RSAKeySize];
        bool _isAESDataEncrypted;
        int _encodedAESDataSize;
        AutoSeededRandomPool _rng;
    };

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::generateRSAKeys() -> void
    {
        _privateKey.GenerateRandomWithKeySize( _rng, RSAKeySize );
        _publicKey = RSA::PublicKey( _privateKey );
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::generateAESKey()
    {
        auto AESData = XLib::view_as< AESData_t* >( _AESData );
        _rng.GenerateBlock( AESData->key, AESKeySize );
        _rng.GenerateBlock( AESData->iv, AESIVSize );

        _isAESDataEncrypted = false;

        return _AESData;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::decryptAESKey()
    {
        if ( _isAESDataEncrypted )
        {
            Integer intAESData( _AESData, _encodedAESDataSize );

            auto decryptAESKey
              = _privateKey.CalculateInverse( _rng, intAESData );

            decryptAESKey.Encode( _AESData,
                                  decryptAESKey.MinEncodedSize() );

            _isAESDataEncrypted = false;

            return true;
        }

        return false;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::encryptAESKey()
    {
        if ( !_isAESDataEncrypted )
        {
            Integer intAESData( _AESData, sizeof( AESData_t ) );

            auto encryptAESKey = _publicKey.ApplyFunction( intAESData );

            _encodedAESDataSize = encryptAESKey.MinEncodedSize();

            encryptAESKey.Encode( _AESData, _encodedAESDataSize );

            _isAESDataEncrypted = true;

            return true;
        }

        return false;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::encrypt( bytes& bs )
    {
        if ( !_isAESDataEncrypted )
        {
            auto AESData = XLib::view_as< AESData_t* >( _AESData );
            CFB_Mode< AES >::Decryption cfbDecryption( AESData->key,
                                                       AESKeySize,
                                                       AESData->iv );

            cfbDecryption.ProcessData( bs.data(), bs.data(), bs.size() );
            return true;
        }

        return false;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::decrypt( bytes& bs )
    {
        if ( !_isAESDataEncrypted )
        {
            auto AESData = XLib::view_as< AESData_t* >( _AESData );
            CFB_Mode< AES >::Encryption cfbEcryption( AESData->key,
                                                      AESKeySize,
                                                      AESData->iv );

            cfbEcryption.ProcessData( bs.data(), bs.data(), bs.size() );
            return true;
        }

        return false;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::AESData() const
    {
        return _AESData;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::setAESData( const AESData_t& AESData )
      -> void
    {
        std::memcpy( _AESData, &AESData, sizeof( AESData_t ) );
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::publicKey() const
    {
        return _publicKey;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::setPublicKey(
      const RSA::PublicKey& publicKey ) -> void
    {
        _publicKey = publicKey;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::privateKey() const
    {
        return _privateKey;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::setPrivateKey(
      const RSA::PrivateKey& privateKey ) -> void
    {
        _privateKey = privateKey;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::debugKeys() -> void
    {
        const auto& nr = _publicKey.GetModulus();
        const auto& er = _publicKey.GetPublicExponent();

        const auto& np    = _privateKey.GetModulus();
        const auto& epub  = _privateKey.GetPublicExponent();
        const auto& epriv = _privateKey.GetPrivateExponent();

        ConsoleOutput( "RSA Public Key: Modulus\n" ) << nr << std::endl;

        ConsoleOutput( "RSA Public Key: PublicExponent\n" )
          << er << std::endl;

        ConsoleOutput( "RSA Private Key: Modulus\n" ) << np << std::endl;

        ConsoleOutput( "RSA Private Key: PublicExponent\n" )
          << epub << std::endl;

        ConsoleOutput( "RSA Private Key: PrivateExponent\n" )
          << epriv << std::endl;

        ConsoleOutput( "AES IV:\n" );

        auto AESData = XLib::view_as< AESData_t* >( _AESData );

        for ( auto b : AESData->iv )
        {
            std::cout << std::hex << static_cast< int >( b );
        }

        std::cout << std::endl;

        ConsoleOutput( "AES Key:\n" );

        for ( auto b : AESData->key )
        {
            std::cout << std::hex << static_cast< int >( b );
        }

        std::cout << std::endl;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::isAESDataEncrypted() const
    {
        return _isAESDataEncrypted;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::setIsAESDataEncrypted(
      bool isAESDataEncrypted ) -> void
    {
        _isAESDataEncrypted = isAESDataEncrypted;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::encodedAESDataSize() const -> int
    {
        return _encodedAESDataSize;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::setEncodedAESDataSize(
      int encodedAESDataSize ) -> void
    {
        _encodedAESDataSize = encodedAESDataSize;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::rng() const
    {
        return _rng;
    }
    template < int RSAKeySize >
    auto
    HybridCrypt< RSAKeySize >::setRng( const AutoSeededRandomPool& rng )
      -> void
    {
        _rng = rng;
    }

}

#endif // HYBRIDCRYPT_H
