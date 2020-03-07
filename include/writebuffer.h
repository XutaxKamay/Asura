#ifndef WRITEBUFFER_H
#define WRITEBUFFER_H

#include "buffer.h"

namespace XLib
{
    using namespace std;

    template <safesize_t max_size = 0>
    /**
     * @brief The WriteBuffer class
     * Write types and data to the buffer.
     * max_size stands for maximum size for the buffer.
     * Example
     * WriteBuffer<1024> writeBuffer;
     * writeBuffer.addVar<type_32>(1337);
     */
    class WriteBuffer : public Buffer<max_size>
    {
       public:
        /**
         * @brief WriteBuffer
         */
        WriteBuffer();
        /**
         * @brief WriteBuffer
         * @param pData
         * @param bAllocated
         * @param writeSize
         * @param maxSize
         */
        explicit WriteBuffer(array_t pData,
                             bool allocated = false,
                             safesize_t writeSize = 0,
                             safesize_t maxSize = 0);

        ~WriteBuffer() = default;

       public:
        /**
         * @brief addType
         * @param typeSize
         */
        auto addType(typesize_t typeSize) -> void;
        /**
         * @brief reset
         */
        auto reset() -> void;
        /**
         * @brief addData
         * @param pData
         * @param size
         */
        auto addData(ptr_t pData, safesize_t size) -> void;
        /**
         * @brief advance
         * @param size
         */
        auto advance(safesize_t size) -> void;
        /**
         * @brief writeSize
         * @return
         */
        auto writeSize() const;
        /**
         * @brief setWriteSize
         * @param writeSize
         */
        auto setWriteSize(const safesize_t& writeSize) -> void;
        /**
         * @brief toBytes
         */
        auto toBytes();

        /** templates */
       public:
        template <typesize_t typeSize = type_32>
        /**
         * @brief addVar
         * @param value
         * @param size
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
        /**
         * @brief shift
         * @param size
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
        /**
         * @brief m_writeSize
         */
        safesize_t _writeSize {};
    };

    template <safesize_t max_size>
    WriteBuffer<max_size>::WriteBuffer() : Buffer<max_size>()
    {}

    template <safesize_t max_size>
    WriteBuffer<max_size>::WriteBuffer(array_t pData,
                                       bool allocated,
                                       safesize_t writeSize,
                                       safesize_t maxSize) :
        Buffer<max_size>(pData, allocated, maxSize)
    {
        _writeSize = writeSize;
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::addType(typesize_t typeSize) -> void
    {
        addData(&typeSize, static_cast<safesize_t>(sizeof(typeSize)));
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::reset() -> void
    {
        _writeSize = 0;
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::addData(ptr_t pData, safesize_t size)
        -> void
    {
        memcpy(shift(_writeSize), pData, static_cast<size_t>(size));
        advance(size);
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::advance(safesize_t size) -> void
    {
        _writeSize += size;
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::writeSize() const
    {
        return _writeSize;
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::setWriteSize(const safesize_t& writeSize)
        -> void
    {
        _writeSize = writeSize;
    }

    template <safesize_t max_size>
    inline auto WriteBuffer<max_size>::toBytes()
    {
        bytes_t bs(_writeSize);
        copy(this->_pData, this->_pData + _writeSize, bs.begin());

        return bs;
    }

}

#endif // WRITEBUFFER_H
