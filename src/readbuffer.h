#ifndef XKLIB_READBUFFER_H
#define XKLIB_READBUFFER_H

#include "buffer.h"
#include "exception.h"

namespace XKLib
{
    class ReadBuffer : public Buffer
    {
      public:
        explicit ReadBuffer(const data_t data,
                            const std::size_t maxSize  = 0,
                            const std::size_t readSize = 0);

      public:
        auto readSize() const -> const std::size_t&;

      public:
        auto readSize() -> std::size_t&;
        auto reset() -> void;
        auto advance(const std::size_t size) -> void;

      public:
        template <TypeSize T = type_32s>
        constexpr inline auto readVar(std::size_t* const pSize = nullptr)
        {
            if (_read_size >= maxSize())
            {
                XKLIB_EXCEPTION("Filled buffer");
            }

            auto type = *this->shift<TypeSize*>(_read_size);
            advance(sizeof(TypeSize));

            /* Read type first */
            if (type != T)
            {
                /**
                 * Blame programmer for not writing the buffer
                 * correctly.
                 */
                XKLIB_EXCEPTION(
                  std::string("Expected type: " + get_variable_type_str(T)
                              + "when type is instead "
                              + get_variable_type_str(type)));
            }

            using var_t = get_variable_t<T>;

            /* Initialize data type */
            var_t data = {};

            if constexpr (T == type_array)
            {
                /* If it's an array we read first its size */
                auto dataSize = *this->shift<std::size_t*>(_read_size);
                advance(sizeof(std::size_t));

                /**
                 * Then we give the pointer of where is located data
                 */
                data = this->shift<var_t>(_read_size);
                advance(dataSize);

                /**
                 * If the parameter isn't null we give the array size
                 */
                if (pSize != nullptr)
                {
                    *pSize = dataSize;
                }
            }
            else
            {
                data = *this->shift<var_t*>(_read_size);
                advance(sizeof(var_t));

                /**
                 * If the parameter isn't null we give the type size
                 */
                if (pSize != nullptr)
                {
                    *pSize = sizeof(var_t);
                }
            }

            return data;
        }

        template <typename T = ptr_t>
        constexpr inline auto shift(const std::size_t size = 0) -> T
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
        std::size_t _read_size;
    };

}

#endif
