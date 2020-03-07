#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <vector>
#include <cstddef>

namespace XLib
{
    using ptr_t      = void*;
    using byte_t     = unsigned char;
    using array_t    = byte_t*;
    using bytes_t    = std::vector< byte_t >;
    using safesize_t = int32_t;
}

#define ConsoleOutput( format, ... )                                            \
    std::cout << ( std::string( std::string( "[XLib] -> " ) + format ).c_str(), \
                   ##__VA_ARGS__ )

#endif // TYPES_H
