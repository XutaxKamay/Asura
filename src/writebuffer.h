#ifndef WRITEBUFFER_H
#define WRITEBUFFER_H

#include "buffer.h"

namespace XLib
{
    /**
     * @brief The WriteBuffer class
     * Write types and data to the buffer.
     */
    class WriteBuffer : public Buffer
    {
      public:
        constexpr static auto DEFAULT_MAX_SIZE = 0x100000;

        WriteBuffer(data_t data          = nullptr,
                    safesize_t writeSize = 0,
                    safesize_t maxSize   = DEFAULT_MAX_SIZE);

        ~WriteBuffer() = default;

      public:
        /**
         * @brief addType
         * @param typesize_T
         */
        auto addType(typesize_t typesize_T) -> void;
        /**
         * @brief reset
         */
        auto reset() -> void;
        /**
         * @brief addData
         * @param data
         * @param size
         */
        auto addData(ptr_t data, safesize_t size) -> void;
        /**
         * @brief advance
         * @param size
         */
        auto advance(safesize_t size) -> void;
        /**
         * @brief writeSize
         * @return
         */
        auto writeSize() -> safesize_t;
        /**
         * @brief setWriteSize
         * @param writeSize
         */
        auto setWriteSize(const safesize_t& writeSize) -> void;
        /**
         * @brief toBytes
         */
        auto toBytes() -> bytes_t;

        /** templates */
      public:
        template <typesize_t typesize_T = type_32s>
        /**
         * @brief addVar
         * @param value
         * @param size
         */
        constexpr inline auto addVar(get_variable_t<typesize_T> value,
                                     safesize_t size = 0)
        {
            /* Add first the type of variable */
            addType(typesize_T);

            if constexpr (typesize_T == type_array)
            {
                /* Add the size of the array if it's an array */
                addData(&size, view_as<safesize_t>(sizeof(size)));
                addData(value, view_as<safesize_t>(size));
            }
            else
            {
                addData(&value, view_as<safesize_t>(sizeof(value)));
            }
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

      private:
        /**
         * @brief m_written_size
         */
        safesize_t _written_size {};
    };

} // namespace XLib

#endif // WRITEBUFFER_H
