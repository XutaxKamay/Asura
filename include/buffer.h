#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <iostream>
#include <vector>

namespace XLib
{
    /*!
     * \brief UDPSize
     * UDPSize is the maximum size theorically that we can send over network on
     * UDP protocol without fragmentation
     */
    constexpr auto UDPSize = 508;

    using ptr_t = void*;
    using byte_t = unsigned char;
    using array_t = byte_t*;
    using bytes_t = std::vector<byte_t>;
    using safesize_t = int32_t;

    template <typename T>
    struct type_wrapper_t
    {
        using type = T;
    };

    template <typename T>
    inline constexpr type_wrapper_t<T> _type {};

    template <typename T>
    /*!
     * \brief alloc
     * \param size
     */
    constexpr inline auto alloc(safesize_t size);

    template <typename T>
    /*!
     * \brief free
     * \param pBuf
     */
    constexpr inline void free(T& pBuf);

    /*!
     * \brief The typesize_t enum
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

    template <typesize_t type>
    /*!
     * \brief _gvt
     * _gvt (get variable type) permits to get variable type from typesize_t.
     */
    constexpr inline auto _gvt();

    /*!
     * \brief gvtStr
     * \param typeSize
     * \return Returns the string of the variable type.
     */
    inline std::string gvtStr(typesize_t typeSize);

    template <typesize_t _type>
    using gvt = typename decltype(_gvt<_type>())::type;

    template <safesize_t max_size = 0>
    /*!
     * \brief The Buffer class
     * Just a simple class for allocating bytes and buffer purposes.
     * usage: Buffer<1100> buffer;
     */
    class Buffer
    {
       public:
        /*!
         * \brief Buffer
         */
        Buffer();

        /*!
         * \brief Buffer
         * \param pData
         * \param allocated
         * \param maxSize
         */
        Buffer(array_t pData, bool allocated, safesize_t maxSize = 0);

        ~Buffer();

       public:
        /*!
         * \brief operator []
         * \param size
         * \return
         */
        constexpr inline auto& operator[](safesize_t size);

        /*!
         * \brief pData
         * \return
         */
        constexpr inline array_t pData() const;

        /*!
         * \brief setPData
         * \param pData
         */
        constexpr inline void setPData(const array_t& pData);

        /*!
         * \brief maxSize
         * \return
         */
        constexpr inline safesize_t maxSize() const;

        /*!
         * \brief setMaxSize
         * \param maxSize
         */
        constexpr inline void setMaxSize(const safesize_t& maxSize);

       public:
        template <typename cast_t = ptr_t>
        /*!
         * \brief shift
         * \param size
         * Gets a pointer from data + size.
         */
        constexpr inline auto shift(safesize_t size)
        {
            if (size == 0)
            {
                return reinterpret_cast<cast_t>(_pData);
            }
            else
            {
                return reinterpret_cast<cast_t>(
                    reinterpret_cast<uintptr_t>(_pData) + size);
            }
        }

       private:
        /*!
         * \brief _pData
         */
        array_t _pData {};
        /*!
         * \brief _maxSize
         */
        safesize_t _maxSize;
        /*!
         * \brief _allocated
         */
        bool _allocated;
    };
}

#endif // BUFFER_H
