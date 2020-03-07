#ifndef BUFFER_H
#define BUFFER_H

#include "types.h"

namespace XLib
{
    /**
     * @brief UDPSize
     * UDPSize is the maximum size theorically that we can send over network on
     * UDP protocol without fragmentation
     */
    constexpr auto UDPSize = 508;

    template < typename T >
    struct type_wrapper_t
    {
        using type = T;
    };

    template < typename T >
    inline constexpr type_wrapper_t< T > _type {};

    template < typename T >
    /**
     * @brief alloc
     * @param size
     */
    constexpr inline auto alloc( safesize_t size )
    {
        return reinterpret_cast< T >(
          ::operator new( static_cast< size_t >( size ) ) );
    }

    template < typename T >
    /**
     * @brief free
     * @param pBuf
     */
    constexpr inline void free( T& pBuf )
    {
        ::operator delete( reinterpret_cast< ptr_t >( pBuf ) );
    }
    /**
     * @brief The typesize_t enum
     * Enumerate all kind of primitive types that could be possibly used in a
     * buffer.
     */
    enum typesize_t : byte_t
    {
        type_float,
        type_double,
        type_safesize,
        type_32,
        type_64,
        type_16,
        type_8,
        type_array,
        type_unknown
    };

    template < typesize_t type >
    /**
     * @brief _gvt
     * _gvt (get variable type) permits to get variable type from typesize_t.
     */
    constexpr inline auto _gvt()
    {
        if constexpr ( type == type_safesize )
            return _type< safesize_t >;
        else if constexpr ( type == type_8 )
            return _type< byte_t >;
        else if constexpr ( type == type_16 )
            return _type< uint16_t >;
        else if constexpr ( type == type_32 )
            return _type< uint32_t >;
        else if constexpr ( type == type_64 )
            return _type< uint64_t >;
        else if constexpr ( type == type_array )
            return _type< array_t >;
        else if constexpr ( type == type_float )
            return _type< float >;
        else if constexpr ( type == type_double )
            return _type< double >;
        else
            return _type< void >;
    }
    /**
     * @brief gvtStr
     * @param typeSize
     * @return Returns the string of the variable type.
     */
    std::string gvtStr( typesize_t typeSize );

    template < typesize_t _type >
    using gvt = typename decltype( _gvt< _type >() )::type;
    template < typesize_t _type >
    using gvt = gvt< _type >;

    template < safesize_t max_size = 0 >
    /**
     * @brief The Buffer class
     * Just a simple class for allocating bytes and buffer purposes.
     * usage: Buffer<1100> buffer;
     */
    class Buffer
    {
      public:
        /**
         * @brief Buffer
         */
        Buffer();
        /**
         * @brief Buffer
         * @param pData
         * @param allocated
         * @param maxSize
         */
        Buffer( array_t pData, bool allocated, safesize_t maxSize = 0 );

        ~Buffer();

      public:
        /**
         * @brief operator []
         * @param size
         * @return
         */
        auto& operator[]( safesize_t size );
        /**
         * @brief pData
         * @return
         */
        auto pData() const;
        /**
         * @brief setPData
         * @param pData
         */
        auto setPData( const array_t& pData );
        /**
         * @brief maxSize
         * @return
         */
        auto maxSize() const;
        /**
         * @brief setMaxSize
         * @param maxSize
         */
        auto setMaxSize( const safesize_t& maxSize );
        /**
         * @brief toBytes
         * @return bytes_t
         */
        auto toBytes();

      public:
        template < typename cast_t = ptr_t >
        /**
         * @brief shift
         * @param size
         * Gets a pointer from data + size.
         */
        constexpr inline auto shift( safesize_t size )
        {
            if ( size == 0 )
            {
                return reinterpret_cast< cast_t >( _pData );
            }
            else
            {
                return reinterpret_cast< cast_t >(
                  reinterpret_cast< uintptr_t >( _pData ) + size );
            }
        }

      protected:
        /**
         * @brief _pData
         */
        array_t _pData {};
        /**
         * @brief _maxSize
         */
        safesize_t _maxSize;
        /**
         * @brief _allocated
         */
        bool _allocated;
    };

    template < safesize_t max_size >
    Buffer< max_size >::Buffer()
     : _maxSize( max_size ), _allocated( max_size != 0 )
    {
        // Allocate with the maximum size for performance.
        if ( max_size != 0 )
            _pData = alloc< decltype( _pData ) >( max_size );
    }

    template < safesize_t max_size >
    Buffer< max_size >::Buffer( array_t pData,
                                bool allocated,
                                safesize_t maxSize )
     : _pData( pData ), _maxSize( maxSize ), _allocated( allocated )
    {}

    template < safesize_t max_size >
    Buffer< max_size >::~Buffer()
    {
        // Free data.
        if ( _allocated )
            free( _pData );
    }

    template < safesize_t max_size >
    auto& Buffer< max_size >::operator[]( safesize_t size )
    {
        return *shift< byte_t* >( size );
    }

    template < safesize_t max_size >
    inline auto Buffer< max_size >::pData() const
    {
        return _pData;
    }

    template < safesize_t max_size >
    inline auto Buffer< max_size >::setPData( const array_t& pData )
    {
        _pData = pData;
    }

    template < safesize_t max_size >
    inline auto Buffer< max_size >::maxSize() const
    {
        return _maxSize;
    }

    template < safesize_t max_size >
    inline auto Buffer< max_size >::setMaxSize( const safesize_t& maxSize )
    {
        _maxSize = maxSize;
    }

    template < safesize_t max_size >
    inline auto Buffer< max_size >::toBytes()
    {
        bytes_t bs( max_size );
        copy( this->_pData, this->_pData + max_size, bs.begin() );

        return bs;
    }

}

#endif // BUFFER_H
