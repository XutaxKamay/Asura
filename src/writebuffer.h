#ifndef XKLIB_WRITEBUFFER_H
#define XKLIB_WRITEBUFFER_H

#include "buffer.h"

namespace XKLib
{
    /**
     * Write types and data to the buffer.
     */
    class WriteBuffer : public Buffer
    {
      public:
        constexpr static auto DEFAULT_MAX_SIZE = 0x100000;

        explicit WriteBuffer(const data_t data         = nullptr,
                             const std::size_t maxSize = DEFAULT_MAX_SIZE,
                             const std::size_t writeSize = 0);

        ~WriteBuffer() = default;

      public:
        auto writeSize() const -> const std::size_t&;
        auto toBytes() const -> bytes_t;

      public:
        auto writeSize() -> std::size_t&;
        auto addType(const TypeSize T) -> void;
        auto reset() -> void;
        auto addData(const ptr_t data, const std::size_t size) -> void;
        auto advance(const std::size_t size) -> void;

      public:
        template <TypeSize T = type_32s>
        constexpr inline auto addVar(get_variable_t<T> value,
                                     std::size_t size = 0)
        {
            /* Add first the type of variable */
            addType(T);

            if constexpr (T == type_array)
            {
                /* Add the size of the array if it's an array */
                addData(&size, view_as<std::size_t>(sizeof(size)));
                addData(value, view_as<std::size_t>(size));
            }
            else
            {
                addData(&value, view_as<std::size_t>(sizeof(value)));
            }
        }

        template <typename T = ptr_t>
        constexpr inline auto shift(const std::size_t size = 0)
        {
            if (size == 0)
            {
                return view_as<T>(this->data());
            }
            else
            {
                return view_as<T>(view_as<std::uintptr_t>(this->data())
                                  + view_as<std::uintptr_t>(size));
            }
        }

      private:
        std::size_t _written_size {};
    };

}

#endif
