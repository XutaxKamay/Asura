#include "writebuffer.h"

using namespace XLib;
using namespace std;

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
constexpr inline auto WriteBuffer<max_size>::addType(typesize_t typeSize)
{
    addData(&typeSize, static_cast<safesize_t>(sizeof(typeSize)));
}

template <safesize_t max_size>
constexpr inline auto WriteBuffer<max_size>::reset()
{
    _writeSize = 0;
}

template <safesize_t max_size>
constexpr inline auto WriteBuffer<max_size>::addData(ptr_t pData,
                                                     safesize_t size)
{
    copy(shift(_writeSize), pData, static_cast<size_t>(size));
    advance(size);
}

template <safesize_t max_size>
constexpr inline auto WriteBuffer<max_size>::advance(safesize_t size)
{
    _writeSize += size;
}

template <safesize_t max_size>
constexpr inline auto WriteBuffer<max_size>::writeSize() const
{
    return _writeSize;
}

template <safesize_t max_size>
constexpr inline auto
WriteBuffer<max_size>::setWriteSize(const safesize_t& writeSize)
{
    _writeSize = writeSize;
}
