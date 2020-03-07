#include "readbuffer.h"
using namespace XLib;

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
constexpr inline auto ReadBuffer<max_size>::reset()
{
    _readSize = 0;
}

template <safesize_t max_size>
constexpr inline auto ReadBuffer<max_size>::advance(safesize_t size)
{
    _readSize += size;
}

template <safesize_t max_size>
constexpr inline auto ReadBuffer<max_size>::readSize() const
{
    return _readSize;
}

template <safesize_t max_size>
constexpr inline auto
ReadBuffer<max_size>::setReadSize(const safesize_t& readSize)
{
    _readSize = readSize;
}
