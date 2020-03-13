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
#include "types.h"
#include <sstream>

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
            byte iv[ AESIVSize ];
            byte key[ AESKeySize ];
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
        AESData_t _AESData {};
        byte _encryptedAESData[ RSAKeySize ];
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
        _rng.GenerateBlock( _AESData.key, AESKeySize );
        _rng.GenerateBlock( _AESData.iv, AESIVSize );

        _isAESDataEncrypted = false;

        return _AESData;
    }

    template < int RSAKeySize >
    auto HybridCrypt< RSAKeySize >::decryptAESKey()
    {
        if ( _isAESDataEncrypted )
        {
            auto bytes = reinterpret_cast< byte* >( &_encryptedAESData );
            Integer intAESData( bytes, _encodedAESDataSize );

            auto decryptAESKey
              = _privateKey.CalculateInverse( _rng, intAESData );

            byte decodedBytes[ decryptAESKey.MinEncodedSize() ];

            decryptAESKey.Encode( decodedBytes,
                                  decryptAESKey.MinEncodedSize() );

            std::memcpy( &_AESData, decodedBytes, sizeof( AESData_t ) );

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
            Integer intAESData( reinterpret_cast< byte* >( &_AESData ),
                                sizeof( AESData_t ) );

            auto encryptAESKey = _publicKey.ApplyFunction( intAESData );

            _encodedAESDataSize = encryptAESKey.MinEncodedSize();

            encryptAESKey.Encode( _encryptedAESData,
                                  _encodedAESDataSize );

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
            CFB_Mode< AES >::Decryption cfbDecryption( _AESData.key,
                                                       AESKeySize,
                                                       _AESData.iv );

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
            CFB_Mode< AES >::Encryption cfbEcryption( _AESData.key,
                                                      AESKeySize,
                                                      _AESData.iv );

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
    auto
    HybridCrypt< RSAKeySize >::setAESData( const AESData_t& AESData_t )
      -> void
    {
        _AESData = AESData_t;
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

        for ( auto b : _AESData.iv )
        {
            std::cout << std::hex << static_cast< int >( b );
        }

        std::cout << std::endl;

        ConsoleOutput( "AES Key:\n" );

        for ( auto b : _AESData.key )
        {
            std::cout << std::hex << static_cast< int >( b );
        }

        std::cout << std::endl;

        ConsoleOutput( "AES Encrypted:\n" );

        for ( auto b : _encryptedAESData )
        {
            std::cout << std::hex << static_cast< int >( b );
        }

        std::cout << std::endl;
    }

}

#endif // HYBRIDCRYPT_H
