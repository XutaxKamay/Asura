#ifndef WRITEBUFFER_H
#define WRITEBUFFER_H

#include "buffer.h"

namespace XLib
{
    template <safesize_t max_size = 0>
    /*!
     * \brief The WriteBuffer class
     * Write types and data to the buffer.
     * max_size stands for maximum size for the buffer.
     * Example
     * WriteBuffer<1024> writeBuffer;
     * writeBuffer.addType<type_32>(1337);
     */
    class WriteBuffer : public Buffer<max_size>
    {
       public:
        /*!
         * \brief WriteBuffer
         */
        WriteBuffer();
        /*!
         * \brief WriteBuffer
         * \param pData
         * \param bAllocated
         * \param writeSize
         * \param maxSize
         */
        explicit WriteBuffer(array_t pData,
                             bool allocated = false,
                             safesize_t writeSize = 0,
                             safesize_t maxSize = 0);

        ~WriteBuffer() = default;

       public:
        /*!
         * \brief addType
         * \param typeSize
         */
        constexpr inline auto addType(typesize_t typeSize);
        /*!
         * \brief reset
         */
        constexpr inline auto reset();
        /*!
         * \brief addData
         * \param pData
         * \param size
         */
        constexpr inline auto addData(ptr_t pData, safesize_t size);
        /*!
         * \brief advance
         * \param size
         */
        constexpr inline auto advance(safesize_t size);
        /*!
         * \brief writeSize
         * \return
         */
        constexpr inline auto writeSize() const;
        /*!
         * \brief setWriteSize
         * \param writeSize
         */
        constexpr inline auto setWriteSize(const safesize_t& writeSize);

        /*! templates */
       public:
        template <typesize_t typeSize = type_32>
        /*!
         * \brief addVar
         * \param value
         * \param size
         */
        constexpr inline auto addVar(gvt<typeSize> value, safesize_t size = 0)
        {
            /* Add first the type of variable */
            addType(typeSize);

            if constexpr (typeSize == type_array)
            {
                /* Add the size of the array if it's an array */
                addData(&size, static_cast<safesize_t>(sizeof(size)));
                addData(value, static_cast<safesize_t>(size));
            }
            else
            {
                addData(&value, static_cast<safesize_t>(sizeof(value)));
            }
        }

        template <typename cast_t = ptr_t>
        /*!
         * \brief shift
         * \param size
         */
        constexpr inline auto shift(safesize_t size = 0)
        {
            if (size == 0)
            {
                return reinterpret_cast<cast_t>(this->pData());
            }
            else
            {
                return reinterpret_cast<cast_t>(
                    reinterpret_cast<uintptr_t>(this->pData()) +
                    static_cast<uintptr_t>(size));
            }
        }

       private:
        /*!
         * \brief m_writeSize
         */
        safesize_t _writeSize {};
    };
}

#endif // WRITEBUFFER_H
