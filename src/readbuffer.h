#ifndef READBUFFER_H
#define READBUFFER_H

#include "buffer.h"
#include "exception.h"

namespace XLib
{
    class ReadBuffer : public Buffer
    {
      public:
        ReadBuffer(data_t data,
                   safesize_t readSize = 0,
                   safesize_t maxSize  = 0);

        ~ReadBuffer() = default;

      public:
        template <typesize_t typesize_T = type_32s>
        /**
         * @brief readVar
         * @param pSize
         */
        constexpr inline auto readVar(safesize_t* pSize = nullptr)
        {
            auto type = *this->shift<typesize_t*>(_read_size);
            advance(sizeof(typesize_t));

            /* Read type first */
            if (type != typesize_T)
            {
                /*
                 * Blame programmer for not writing the buffer correctly.
                 */
                throw XLIB_EXCEPTION(std::string(
                  "Expected type: " + get_variable_type_str(typesize_T)
                  + "when type is instead "
                  + get_variable_type_str(type)));
            }

            using var_t = get_variable_t<typesize_T>;

            /* Initialize data type */
            var_t data = {};

            if constexpr (typesize_T == type_array)
            {
                /* If it's an array we read first its size */
                auto dataSize = *this->shift<safesize_t*>(_read_size);
                advance(sizeof(safesize_t));

                /* Then we give the pointer of where is located data */
                data = this->shift<var_t>(_read_size);
                advance(dataSize);

                /*
                 * If it the parameter isn't null we give the array size
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

                /* If it the parameter isn't null we give the type size */
                if (pSize != nullptr)
                {
                    *pSize = sizeof(var_t);
                }
            }

            return data;
        }

        template <typename cast_T = ptr_t>
        /**
         * @brief shift
         * @param size
         */
        constexpr inline auto shift(safesize_t size = 0) -> cast_T
        {
            if (size == 0)
            {
                return view_as<cast_T>(this->data());
            }
            else
            {
                return view_as<cast_T>(view_as<uintptr_t>(this->data())
                                       + view_as<uintptr_t>(size));
            }
        }

      public:
        /**
         * @brief reset
         */
        auto reset() -> void;
        /**
         * @brief advance
         * @param size
         */
        auto advance(safesize_t size) -> void;
        /**
         * @brief readSize
         */
        auto readSize() -> safesize_t;
        /**
         * @brief setReadSize
         * @param readSize
         */
        auto setReadSize(const safesize_t& readSize);

      private:
        /**
         * @brief _read_size
         */
        safesize_t _read_size {};
    };

} // namespace XLib

#endif
