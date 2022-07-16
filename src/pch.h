#ifndef XKLIB_PCH_H
#define XKLIB_PCH_H

/* std */
#include <algorithm>
#include <array>
#include <bit>
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
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

/* ELFIO, must be included early */
#include <elfio/elfio.hpp>

#if defined(_WIN32) or defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #define WINDOWS
    #if _WIN64
        #define ENVIRONMENT64
    #else
        #define ENVIRONMENT32
    #endif
#elif defined(__GNUC__)
    #if __x86_64__ or __ppc64__
        #define ENVIRONMENT64
    #else
        #define ENVIRONMENT32
    #endif
#endif

/* OS specific stuffs */
#ifndef WINDOWS
    #include <dlfcn.h>
    #include <fcntl.h>

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
#if defined(__x86_64__) or defined(__i386__)
    #include <x86intrin.h>
#endif

/* CryptoPP */
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/randpool.h>
#include <cryptopp/rdrand.h>
#include <cryptopp/rng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/sha.h>
#include <cryptopp/zlib.h>

#endif
