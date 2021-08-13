#ifndef BUFFER_H
#define BUFFER_H

#include "bits.h"
#include "types.h"
#include <algorithm>

namespace XLib
{
#ifdef DEBUG
    inline std::vector<ptr_t> tracking_memory_allocs;
#endif

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
        auto ptr = view_as<T>(::operator new(view_as<size_t>(size)));

#ifdef DEBUG
        tracking_memory_allocs.push_back(view_as<ptr_t>(ptr));
#endif

        return ptr;
    }

    template <typename T = ptr_t>
    /**
     * @brief free
     * @param pBuf
     */
    constexpr inline auto free(T& pBuf)
    {
#ifdef DEBUG
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
#else
        ::operator delete(view_as<ptr_t>(pBuf));
#endif
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
        type_32us,
        type_64us,
        type_16us,
        type_8us,
        type_32s,
        type_64s,
        type_16s,
        type_8s,
        type_array
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
        else if constexpr (type == type_8us)
            return type_wrapper<byte_t>;
        else if constexpr (type == type_16us)
            return type_wrapper<uint16_t>;
        else if constexpr (type == type_32us)
            return type_wrapper<uint32_t>;
        else if constexpr (type == type_64us)
            return type_wrapper<uint64_t>;
        else if constexpr (type == type_8s)
            return type_wrapper<char>;
        else if constexpr (type == type_16s)
            return type_wrapper<int16_t>;
        else if constexpr (type == type_32s)
            return type_wrapper<int32_t>;
        else if constexpr (type == type_64s)
            return type_wrapper<int64_t>;
        else if constexpr (type == type_array)
            return type_wrapper<data_t>;
        else if constexpr (type == type_float)
            return type_wrapper<float>;
        else if constexpr (type == type_double)
            return type_wrapper<double>;
        else
            static_assert("Not implemented");
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
    using g_v_t = get_variable_t<typesize_T>;

    class Buffer
    {
      public:
        Buffer(size_t maxSize);
        Buffer(data_t data = nullptr, safesize_t maxSize = 0);
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
        auto data() -> data_t;
        /**
         * @brief setData
         * @param data
         */
        auto setData(const data_t& data);
        /**
         * @brief maxSize
         * @return
         */
        auto maxSize() -> safesize_t;
        /**
         * @brief setMaxSize
         * @param maxSize
         */
        auto setMaxSize(const safesize_t& maxSize);
        /**
         * @brief toBytes
         * @return bytes_t
         */
        auto toBytes() -> bytes_t;

      public:
        template <typename cast_T = ptr_t>
        /**
         * @brief shift
         * @param size
         * Gets a pointer from data + size.
         */
        constexpr inline auto shift(safesize_t size = 0)
        {
            if (!_data && size >= _max_size)
            {
                XLIB_EXCEPTION("Out of bounds.");
            }

            return view_as<cast_T>(view_as<uintptr_t>(_data) + size);
        }

      protected:
        /**
         * @brief _data
         */
        data_t _data {};
        /**
         * @brief _max_size
         */
        safesize_t _max_size {};
        /**
         * @brief _allocated
         */
        bool _allocated {};
    };

} // namespace XLib

#endif // BUFFER_H
