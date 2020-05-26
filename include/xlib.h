#ifndef XLIB_H
#define XLIB_H

#include "circularbuffer.h"
#include "detour.h"
#include "hybridcrypto.h"
#include "memoryscanner.h"
#include "process.h"
#include "readbuffer.h"
#include "types.h"
#include "virtualtabletools.h"
#include "writebuffer.h"

#ifndef WINDOWS
    #include <sys/ioctl.h>

    #include "communicate_structs.h"
#endif

#endif // XLIB_H
