#ifndef XLIB_H
#define XLIB_H

#include <cstddef>
#include <iostream>
#include <vector>

#include "hybridcrypto.h"
#include "readbuffer.h"
#include "writebuffer.h"
#include "circularbuffer.h"
#include "detour.h"

#define ConsoleOutput( format, ... )                                            \
    std::cout << ( std::string( std::string( "[XLib] -> " ) + format ).c_str(), \
                   ##__VA_ARGS__ )

namespace XLib
{
        using ptr_t      = void*;
    using byte_t     = unsigned char;
    using array_t    = byte_t*;
    using bytes_t    = std::vector< byte_t >;
}

#endif // XLIB_H
