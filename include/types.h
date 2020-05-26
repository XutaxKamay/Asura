#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <iostream>
#include <vector>

#if defined _WIN32 || defined _WIN64
    #define WINDOWS
#endif

namespace XLib
{
    using ptr_t      = void*;
    using byte_t     = unsigned char;
    using array_t    = byte_t*;
    using bytes_t    = std::vector<byte_t>;
    using safesize_t = int32_t;
    /* linux pid_t */
    using pid_t = uint32_t;
#define __pid_t_defined

    template <typename T>
    struct type_wrapper_t
    {
        using type = T;
    };

    template <typename T>
    inline constexpr type_wrapper_t<T> type_wrapper {};

    template <typename T1, typename T2>
    auto view_as(T2 var)
    {
        return reinterpret_cast<T1>(var);
    }
}

#define ConsoleOutput(format) std::cout << "[XLib] -> " << format

#endif // TYPES_H
