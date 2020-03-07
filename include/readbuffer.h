#ifndef READBUFFER_H
#define READBUFFER_H

#include "buffer.h"
#include <assert.h>

namespace XLib
{
    template <safesize_t max_size = 0>
    /**
     * @brief The ReadBuffer class
     * This class permits to read a buffer easily.
     * Example:
     * ReadBuffer<1024> readBuffer;
     * auto b = readBuffer.readVar<type_8>();
     */
    class ReadBuffer : public Buffer<max_size>
    {
       public:
        /**
         * @brief ReadBuffer
         */
        ReadBuffer();
        /**
         * @brief ReadBuffer
         * @param pData
         * @param allocated
         * @param readSize
         * @param maxSize
         */
        explicit ReadBuffer(array_t pData,
                            bool allocated = false,
                            safesize_t readSize = 0,
                            safesize_t maxSize = 0);

        ~ReadBuffer() = default;

       public:
        template <typesize_t typeSize = type_32>
        /**
         * @brief readVar
         * @param pSize
         */
        constexpr inline auto readVar(safesize_t* pSize = nullptr)
        {
            auto type = *this->shift<typesize_t*>(_readSize);
            advance(sizeof(typesize_t));

            /* Read type first */
            if (type != typeSize)
                /* Blame programmer for not writing the buffer correctly. */
                assert(std::string("Expected type: " + gvtStr(typeSize) +
                                   "when type is instead " + gvtStr(type))
                           .c_str());

            using varType = gvt<typeSize>;

            /* Initialize data type */
            varType data = {};
            if constexpr (typeSize == type_array)
            {
                /* If it's an array we read first its size */
                auto dataSize = *this->shift<safesize_t*>(_readSize);
                advance(sizeof(safesize_t));

                /* Then we give the pointer of where is located data */
                data = this->shift<varType>(_readSize);
                advance(dataSize);

                /* If it the parameter isn't null we give the array size */
                if (pSize != nullptr)
                {
                    *pSize = dataSize;
                }
            }
            else
            {
                data = *this->shift<varType*>(_readSize);
                advance(sizeof(varType));

                /* If it the parameter isn't null we give the type size */
                if (pSize != nullptr)
                {
                    *pSize = sizeof(varType);
                }
            }

            return data;
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

       public:
        /**
         * @brief reset
         */
        void reset();
        /**
         * @brief advance
         * @param size
         */
        void advance(safesize_t size);
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
         * @brief _readSize
         */
        safesize_t _readSize {};
    };

    template <safesize_t max_size>
    ReadBuffer<max_size>::ReadBuffer() : Buffer<max_size>()
    {}

    template <safesize_t max_size>
    ReadBuffer<max_size>::ReadBuffer(array_t pData,
                                     bool allocated,
                                     safesize_t readSize,
                                     safesize_t maxSize) :
        Buffer<max_size>(pData, allocated, maxSize)
    {
        _readSize = readSize;
    }

    template <safesize_t max_size>
    inline void ReadBuffer<max_size>::reset()
    {
        _readSize = 0;
    }

    template <safesize_t max_size>
    inline void ReadBuffer<max_size>::advance(safesize_t size)
    {
        _readSize += size;
    }

    template <safesize_t max_size>
    inline auto ReadBuffer<max_size>::readSize() const
    {
        return _readSize;
    }

    template <safesize_t max_size>
    inline auto ReadBuffer<max_size>::setReadSize(const safesize_t& readSize)
    {
        _readSize = readSize;
    }

}

#endif
