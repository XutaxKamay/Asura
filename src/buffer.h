#ifndef XKLIB_BUFFER_H
#define XKLIB_BUFFER_H

#include "bits.h"
#include "types.h"

namespace XKLib
{
#ifdef DEBUG
    inline std::vector<ptr_t> tracking_memory_allocs;
#endif

    constexpr auto UDPSize = 508;

    template <typename T = ptr_t>
    constexpr inline auto alloc(const std::size_t size)
    {
        const auto ptr = view_as<T>(::operator new(size));

#ifdef DEBUG
        tracking_memory_allocs.push_back(view_as<ptr_t>(ptr));
#endif

        return ptr;
    }

    constexpr inline auto free(auto& pBuf)
    {
#ifdef DEBUG
        const auto it = std::find(tracking_memory_allocs.begin(),
                                  tracking_memory_allocs.end(),
                                  view_as<ptr_t>(pBuf));

        if (it != tracking_memory_allocs.end())
        {
            ::operator delete(view_as<ptr_t>(pBuf));
            pBuf = nullptr;
            tracking_memory_allocs.erase(it);

            return true;
        }
        else
        {
            return false;
        }
#else
        ::operator delete(view_as<ptr_t>(pBuf));
        pBuf = nullptr;
#endif
    }

    template <typename T = ptr_t>
    constexpr inline auto align_alloc(const std::size_t size,
                                      const std::size_t align)
    {
#ifdef WINDOWS
        return view_as<T>(_aligned_malloc(size, align));
#else
        return view_as<T>(std::aligned_alloc(align, size));
#endif
    }

    constexpr inline auto align_free(auto& pBuf)
    {
#ifdef WINDOWS
        _aligned_free(pBuf);
#else
        std::free(pBuf);
#endif
        pBuf = nullptr;
    }

    enum TypeSize : byte_t
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

    template <TypeSize T>
    constexpr inline auto _gvt()
    {
        static_assert(T <= type_array, "Not implemented");

        if constexpr (T == type_safesize)
            return type_wrapper<std::size_t>;
        else if constexpr (T == type_8us)
            return type_wrapper<byte_t>;
        else if constexpr (T == type_16us)
            return type_wrapper<std::uint16_t>;
        else if constexpr (T == type_32us)
            return type_wrapper<std::uint32_t>;
        else if constexpr (T == type_64us)
            return type_wrapper<std::uint64_t>;
        else if constexpr (T == type_8s)
            return type_wrapper<std::int8_t>;
        else if constexpr (T == type_16s)
            return type_wrapper<std::int16_t>;
        else if constexpr (T == type_32s)
            return type_wrapper<std::int32_t>;
        else if constexpr (T == type_64s)
            return type_wrapper<std::int64_t>;
        else if constexpr (T == type_array)
            return type_wrapper<data_t>;
        else if constexpr (T == type_float)
            return type_wrapper<float>;
        else if constexpr (T == type_double)
            return type_wrapper<double>;
    }

    auto get_variable_type_str(const TypeSize typeSize) -> std::string;

    template <TypeSize T>
    using get_variable_t = typename decltype(_gvt<T>())::type;
    template <TypeSize T>
    using g_v_t = get_variable_t<T>;

    class Buffer
    {
      public:
        explicit Buffer(const std::size_t maxSize);
        explicit Buffer(const data_t data         = nullptr,
                        const std::size_t maxSize = 0);
        ~Buffer();

      public:
        auto operator[](const std::size_t size) const -> const auto&;
        auto data() const -> data_t;
        auto maxSize() const -> std::size_t;
        auto toBytes() const -> bytes_t;

      public:
        auto operator[](const std::size_t size) -> auto&;

      public:
        template <typename T = ptr_t>
        constexpr inline auto shift(const std::size_t size = 0) const
        {
            if (not _data and size >= _max_size)
            {
                XKLIB_EXCEPTION("Out of bounds.");
            }

            return view_as<T>(view_as<std::uintptr_t>(_data) + size);
        }

      private:
        data_t _data;
        std::size_t _max_size;
        bool _allocated;
    };
}

#endif
