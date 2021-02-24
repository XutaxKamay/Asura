#ifndef READBUFFER_H
#define READBUFFER_H

#include "buffer.h"
#include <assert.h>

namespace XLib
{
    template <safesize_t max_size_T = 0>
    /**
     * @brief The ReadBuffer class
     * This class permits to read a buffer easily.
     * Example:
     * ReadBuffer<1024> readBuffer;
     * auto b = readBuffer.readVar<type_8>();
     */
    class ReadBuffer : public Buffer<max_size_T>
    {
      public:
        explicit ReadBuffer(data_t data,
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
                assert(std::string("Expected type: "
                                   + get_variable_type_str(typesize_T)
                                   + "when type is instead "
                                   + get_variable_type_str(type))
                         .c_str());
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
        constexpr inline auto shift(safesize_t size = 0)
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
        auto readSize() const;
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

    template <safesize_t max_size_T>
    ReadBuffer<max_size_T>::ReadBuffer(data_t data,
                                       safesize_t readSize,
                                       safesize_t maxSize)
     : Buffer<max_size_T>(data, true, maxSize), _read_size(readSize)
    {
    }

    template <safesize_t max_size_T>
    inline auto ReadBuffer<max_size_T>::reset() -> void
    {
        _read_size = 0;
    }

    template <safesize_t max_size_T>
    inline auto ReadBuffer<max_size_T>::advance(safesize_t size) -> void
    {
        _read_size += size;
    }

    template <safesize_t max_size_T>
    inline auto ReadBuffer<max_size_T>::readSize() const
    {
        return _read_size;
    }

    template <safesize_t max_size_T>
    inline auto ReadBuffer<max_size_T>::setReadSize(
      const safesize_t& readSize)
    {
        _read_size = readSize;
    }

} // namespace XLib

#endif
