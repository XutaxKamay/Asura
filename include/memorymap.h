#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include "types.h"

namespace XLib
{
    template < typename T >
    class MemoryMap
    {
      public:
        enum protection_t
        {
            READ,
            WRITE,
            EXECUTE
        };

        auto begin();
        auto end();
        auto setAddress( const ptr_t& address ) -> void;
        auto setSize( const size_t& size ) -> void;
        auto& protection();

      private:
        ptr_t _address {};
        size_t _size {};
        protection_t _protection {};
    };

    using map_t = MemoryMap< ptr_t >;

    template < typename T >
    auto MemoryMap< T >::begin()
    {
        return reinterpret_cast< T >( _address );
    }

    template < typename T >
    auto MemoryMap< T >::end()
    {
        reinterpret_cast< T >( reinterpret_cast< uintptr_t >( _address )
                               + _size );
    }

    template < typename T >
    auto MemoryMap< T >::setAddress( const ptr_t& address ) -> void
    {
        _address = address;
    }

    template < typename T >
    auto MemoryMap< T >::setSize( const size_t& size ) -> void
    {
        _size = size;
    }

    template < typename T >
    auto& MemoryMap< T >::protection()
    {
        return _protection;
    }
}

#endif // MEMORYMAP_H
