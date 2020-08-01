#ifndef BUFFER_H
#define BUFFER_H

#include "types.h"
#include <algorithm>

namespace XLib
{
    inline std::vector<ptr_t> tracking_memory_allocs;

    /**
     * @brief UDPSize
     * UDPSize is the maximum size theorically that we can send over
     * network on UDP protocol without fragmentation
     */
    constexpr auto UDPSize = 508;

    template <typename T = ptr_t>
    /**
     * @brief alloc
     * @param size
     */
    constexpr inline auto alloc(safesize_t size)
    {
        auto ptr = view_as<T>(::operator new(static_cast<size_t>(size)));

        tracking_memory_allocs.push_back(view_as<ptr_t>(ptr));

        return ptr;
    }

    template <typename T = ptr_t>
    /**
     * @brief free
     * @param pBuf
     */
    constexpr inline auto free(T& pBuf)
    {
        auto it = std::find(tracking_memory_allocs.begin(),
                            tracking_memory_allocs.end(),
                            view_as<ptr_t>(pBuf));

        if (it != tracking_memory_allocs.end())
        {
            ::operator delete(view_as<ptr_t>(pBuf));
            tracking_memory_allocs.erase(it);

            return true;
        }
        else
        {
            return false;
        }
    }
    /**
     * @brief The typesize_t enum
     * Enumerate all kind of primitive types that could be possibly used
     * in a buffer.
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
    /**
     * @brief _gvt
     * _gvt (get variable type) permits to get variable type from
     * typesize_t.
     */
    constexpr inline auto _gvt()
    {
        if constexpr (type == type_safesize)
            return type_wrapper<safesize_t>;
        else if constexpr (type == type_8)
            return type_wrapper<byte_t>;
        else if constexpr (type == type_16)
            return type_wrapper<uint16_t>;
        else if constexpr (type == type_32)
            return type_wrapper<uint32_t>;
        else if constexpr (type == type_64)
            return type_wrapper<uint64_t>;
        else if constexpr (type == type_array)
            return type_wrapper<array_t>;
        else if constexpr (type == type_float)
            return type_wrapper<float>;
        else if constexpr (type == type_double)
            return type_wrapper<double>;
        else
            return type_wrapper<void>;
    }
    /**
     * @brief get_variable_type_str
     * @param typeSize
     * @return Returns the string of the variable type.
     */
    std::string get_variable_type_str(typesize_t typeSize);

    template <typesize_t typesize_T>
    using get_variable_t = typename decltype(_gvt<typesize_T>())::type;
    template <typesize_t typesize_T>
    using gv_t = get_variable_t<typesize_T>;

    template <safesize_t max_size_T = 0>
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
         * @param data
         * @param allocated
         * @param maxSize
         */
        Buffer(array_t data, bool allocated, safesize_t maxSize = 0);

        ~Buffer();

      public:
        /**
         * @brief operator []
         * @param size
         * @return
         */
        auto& operator[](safesize_t size);
        /**
         * @brief data
         * @return
         */
        auto data() const;
        /**
         * @brief setData
         * @param data
         */
        auto setData(const array_t& data);
        /**
         * @brief maxSize
         * @return
         */
        auto maxSize() const;
        /**
         * @brief setMaxSize
         * @param maxSize
         */
        auto setMaxSize(const safesize_t& maxSize);
        /**
         * @brief toBytes
         * @return bytes_t
         */
        auto toBytes();

      public:
        template <typename cast_T = ptr_t>
        /**
         * @brief shift
         * @param size
         * Gets a pointer from data + size.
         */
        constexpr inline auto shift(safesize_t size)
        {
            if (size == 0)
            {
                return view_as<cast_T>(_data);
            }
            else
            {
                return view_as<cast_T>(view_as<uintptr_t>(_data) + size);
            }
        }

      protected:
        /**
         * @brief _data
         */
        array_t _data {};
        /**
         * @brief _max_size
         */
        safesize_t _max_size;
        /**
         * @brief _allocated
         */
        bool _allocated;
    };

    template <safesize_t max_size_T>
    Buffer<max_size_T>::Buffer()
     : _max_size(max_size_T), _allocated(max_size_T != 0)
    {
        // Allocate with the maximum size for performance.
        if (max_size_T != 0)
            _data = alloc<decltype(_data)>(max_size_T);
    }

    template <safesize_t max_size_T>
    Buffer<max_size_T>::Buffer(array_t data,
                               bool allocated,
                               safesize_t maxSize)
     : _data(data), _max_size(maxSize), _allocated(allocated)
    {
    }

    template <safesize_t max_size_T>
    Buffer<max_size_T>::~Buffer()
    {
        // Free data.
        if (_allocated)
            free(_data);
    }

    template <safesize_t max_size_T>
    auto& Buffer<max_size_T>::operator[](safesize_t size)
    {
        return *shift<byte_t*>(size);
    }

    template <safesize_t max_size_T>
    inline auto Buffer<max_size_T>::data() const
    {
        return _data;
    }

    template <safesize_t max_size_T>
    inline auto Buffer<max_size_T>::setData(const array_t& data)
    {
        _data = data;
    }

    template <safesize_t max_size_T>
    inline auto Buffer<max_size_T>::maxSize() const
    {
        return _max_size;
    }

    template <safesize_t max_size_T>
    inline auto Buffer<max_size_T>::setMaxSize(const safesize_t& maxSize)
    {
        _max_size = maxSize;
    }

    template <safesize_t max_size_T>
    inline auto Buffer<max_size_T>::toBytes()
    {
        bytes_t bs(max_size_T);
        copy(this->_data, this->_data + max_size_T, bs.begin());

        return bs;
    }

}

#endif // BUFFER_H
