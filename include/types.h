#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <vector>
#include <cstddef>

#if defined _WIN32 || defined _WIN64
    #define WINDOWS
#endif

namespace XLib
{
    using ptr_t      = void*;
    using byte_t     = unsigned char;
    using array_t    = byte_t*;
    using bytes_t    = std::vector< byte_t >;
    using safesize_t = int32_t;
    using pid_t      = uint32_t;

    template < typename T >
    struct type_wrapper_t
    {
        using type = T;
    };

    template < typename T >
    inline constexpr type_wrapper_t< T > type_wrapper {};

    template < typename T1, typename T2 >
    auto view_as( T2 var )
    {
        return reinterpret_cast< T1 >( var );
    }

    template < typename T = ptr_t >
    /**
     * @brief VTable
     * @param classPtr
     * Gets a virtual table from a class pointer.
     */
    constexpr auto VTable( T classPtr )
    {
        return *view_as< ptr_t** >( classPtr );
    }

    template < safesize_t index, typename T = ptr_t >
    /**
     * @brief VFunc
     * @param classPtr
     * Gets a virtual function type from a virtual table and an index.
     */
    constexpr inline auto VFuncPtr( ptr_t classPtr )
    {
        return view_as< T >( VTable< ptr_t >( classPtr )[ index ] );
    }

    template < safesize_t index, typename TRetType = void, typename... TArgs >
    /**
     * @brief VFunc
     * @param classPtr
     * Gets a virtual function type for calling process from a virtual
     * table and an index.view_as
     */
    constexpr inline auto VFunc( ptr_t classPtr )
    {
#ifdef WINDOWS
        return VFuncPtr< index,
                         TRetType( __thiscall* )( ptr_t, TArgs... ) >(
          classPtr );
#else
        return VFuncPtr< index, TRetType ( * )( ptr_t, TArgs... ) >(
          classPtr );
#endif
    }

    template < safesize_t index, typename TRetType = void, typename... TArgs >
    /**
     * @brief CVFunc
     * @param classPtr
     * @param Args
     * Calls a virtual function from a virtual table and an index.
     */
    constexpr inline auto CVFunc( ptr_t classPtr, TArgs... Args )
    {
        return VFunc< index, TRetType, TArgs... >( classPtr )( classPtr,
                                                               Args... );
    }
}

#define ConsoleOutput( format ) std::cout << "[XLib] -> " << format

#endif // TYPES_H
