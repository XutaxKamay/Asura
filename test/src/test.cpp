#include "test.h"

using namespace XLib;

inline auto XLib::Test::run() -> void
{
    ConsoleOutput( "Starting test" ) << std::endl;

    /* Let's do the tests */
    HybridCrypt hybridCrypt;

    ConsoleOutput( "Generating AES keys" ) << std::endl;

    hybridCrypt.generateAESKey();

    ConsoleOutput( "Generating RSA keys" ) << std::endl;

    hybridCrypt.generateRSAKeys();

    std::string str( "Life is a game, but you can not restart it."
                     "There might be no happy end.."
                     "But that's why people say to live your day"
                     " like it was the last,"
                     " because the pain "
                     "and the number of scars increase over time." );

    ConsoleOutput( str ) << std::endl;

    WriteBuffer< 4096 > writeBuffer;

    auto strSize = static_cast< safesize_t >( str.size() );

    writeBuffer.addVar< type_array >( view_as< gvt< type_array > >( str.data()
),
                                      strSize );
    writeBuffer.addVar< type_array >( view_as< gvt< type_array > >( str.data()
),
                                      strSize );
    writeBuffer.addVar< type_array >( view_as< gvt< type_array > >( str.data()
),
                                      strSize );

    writeBuffer.addVar< type_8 >( 1 );
    writeBuffer.addVar< type_16 >( 3 );
    writeBuffer.addVar< type_32 >( 3 );
    writeBuffer.addVar< type_64 >( 7 );
    constexpr auto flValue = 42.42f;
    writeBuffer.addVar< type_float >( flValue );
    constexpr auto dlValue = 42.42;
    writeBuffer.addVar< type_double >( dlValue );

    auto bs = writeBuffer.toBytes();

    ConsoleOutput( "Raw bytes:" ) << std::endl;

    for ( auto&& b : bs )
    {
        std::cout << std::hex << static_cast< int >( b );
    }

    std::cout << std::endl;

    ConsoleOutput( "Encrypting..." ) << std::endl;

    /* Let's test the hybrid crypto */
    hybridCrypt.encrypt( bs );

    ConsoleOutput( "Raw bytes:" ) << std::endl;
    for ( auto&& b : bs )
    {
        std::cout << std::hex << static_cast< int >( b );
    }

    std::cout << std::endl;

    ConsoleOutput( "Decrypting..." ) << std::endl;
    hybridCrypt.decrypt( bs );

    ConsoleOutput( "Raw bytes:" ) << std::endl;
    for ( auto&& b : bs )
    {
        std::cout << std::hex << static_cast< int >( b );
    }

    std::cout << std::endl;

    ReadBuffer readBuffer( bs.data() );

    if ( memcmp( readBuffer.pData(),
                 writeBuffer.pData(),
                 static_cast< size_t >( writeBuffer.writeSize() ) )
         == 0 )
    {
        ConsoleOutput( "Passed decryption test" ) << std::endl;
    }
    else
    {
        ConsoleOutput( "Passed decryption test failed" ) << std::endl;
        g_PassedTests = false;
    }

    auto ptr = readBuffer.readVar< type_array >( &strSize );
    ConsoleOutput( std::string( ptr, ptr + strSize ) ) << std::endl;
    ptr = readBuffer.readVar< type_array >( &strSize );
    ConsoleOutput( std::string( ptr, ptr + strSize ) ) << std::endl;
    ptr = readBuffer.readVar< type_array >( &strSize );
    ConsoleOutput( std::string( ptr, ptr + strSize ) ) << std::endl;

    if ( readBuffer.readVar< type_8 >() == 1
         && readBuffer.readVar< type_16 >() == 3
         && readBuffer.readVar< type_32 >() == 3
         && readBuffer.readVar< type_64 >() == 7
         && readBuffer.readVar< type_float >() == flValue
         && readBuffer.readVar< type_double >() == dlValue )
    {
        ConsoleOutput( "Passed reading" ) << std::endl;
    }
    else
    {
        g_PassedTests = false;
    }

    if ( g_PassedTests )
    {
        ConsoleOutput( "Passed all tests" ) << std::endl;
    }
    else
    {
        ConsoleOutput( "Failed test(s)" ) << std::endl;
    }

    /* Virtual Functions Tests */

    CVFunc<0, void>()
}

void XLib::Test::API::func1()
{
    std::cout << "func1" << std::endl;
}

std::vector< int > XLib::Test::API::func2( const char* str, ... )
{
    va_list parameterInfos;
    va_start( parameterInfos, str );

    char buffer[ 0x1000 ];
    sprintf( buffer, str, parameterInfos );

    std::cout << buffer << std::endl;

    va_end( parameterInfos );

    auto ints = view_as< int* >( buffer );

    std::vector< int > result;

    for ( auto i = 0; i < sizeof( buffer ) / sizeof( int ); i++ )
    {
        result.push_back( ints[ i ] );
    }

    return result;
}
