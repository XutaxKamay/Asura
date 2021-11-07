#ifndef WRITEBUFFER_H
#define WRITEBUFFER_H

#include "buffer.h"

namespace XKLib
{
    /**
     * @brief The WriteBuffer class
     * Write types and data to the buffer.
     */
    class WriteBuffer : public Buffer
    {
        public:
            constexpr static auto DEFAULT_MAX_SIZE = 0x100000;

            explicit WriteBuffer(data_t data      = nullptr,
                                 size_t maxSize   = DEFAULT_MAX_SIZE,
                                 size_t writeSize = 0);

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
            auto addData(ptr_t data, size_t size) -> void;
            /**
             * @brief advance
             * @param size
             */
            auto advance(size_t size) -> void;
            /**
             * @brief writeSize
             * @return
             */
            auto writeSize() -> size_t;
            /**
             * @brief setWriteSize
             * @param writeSize
             */
            auto setWriteSize(size_t writeSize) -> void;
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
                                         size_t size = 0)
            {
                /* Add first the type of variable */
                addType(typesize_T);

                if constexpr (typesize_T == type_array)
                {
                    /* Add the size of the array if it's an array */
                    addData(&size, view_as<size_t>(sizeof(size)));
                    addData(value, view_as<size_t>(size));
                }
                else
                {
                    addData(&value, view_as<size_t>(sizeof(value)));
                }
            }

            template <typename cast_T = ptr_t>
            /**
             * @brief shift
             * @param size
             */
            constexpr inline auto shift(size_t size = 0)
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
            size_t _written_size {};
    };

} // namespace XKLib

#endif // WRITEBUFFER_H
