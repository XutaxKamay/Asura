#ifndef PCH_H
#define PCH_H

/* std */
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <concepts>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <sstream>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

/* ELFIO, must be included early */
#include <elfio/elfio.hpp>

#if defined(_WIN32) || defined(_WIN64)
    #define WINDOWS
    #if _WIN64
        #define ENVIRONMENT64
    #else
        #define ENVIRONMENT32
    #endif
#elif defined(__GNUC__)
    #if __x86_64__ || __ppc64__
        #define ENVIRONMENT64
    #else
        #define ENVIRONMENT32
    #endif
#endif

/* OS specific stuffs */
#ifndef WINDOWS
    #include <dlfcn.h>
    #include <fcntl.h>
    #include <link.h>

    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <sys/wait.h>

    #include <linux/limits.h>
#else
    #include <windows.h>

    #include <dbghelp.h>
    /* fuck windows not supporting std::aligned_alloc */
    #include <malloc.h>
    #include <psapi.h>
    #include <tlhelp32.h>
#endif

/* SIMD */
#include <x86intrin.h>

/* CryptoPP */
#include "vendor/cryptopp/aes.h"
#include "vendor/cryptopp/modes.h"
#include "vendor/cryptopp/osrng.h"
#include "vendor/cryptopp/randpool.h"
#include "vendor/cryptopp/rdrand.h"
#include "vendor/cryptopp/rng.h"
#include "vendor/cryptopp/rsa.h"
#include "vendor/cryptopp/sha.h"
#include "vendor/cryptopp/zlib.h"

#endif

