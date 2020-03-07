#ifndef XLIB_H
#define XLIB_H

#include "hybridcrypto.h"
#include "readbuffer.h"
#include "writebuffer.h"

#define ConsoleOutput( format, ... )                                            \
    std::cout << ( std::string( std::string( "[XLib] -> " ) + format ).c_str(), \
                   ##__VA_ARGS__ )

namespace XLib
{}

#endif // XLIB_H
