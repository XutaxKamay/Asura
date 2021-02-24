#include "readbuffer.h"

using namespace XLib;

ReadBuffer::ReadBuffer(data_t data,
                       safesize_t readSize,
                       safesize_t maxSize)
 : Buffer(data, maxSize), _read_size(readSize)
{
}

auto ReadBuffer::reset() -> void
{
    _read_size = 0;
}

auto ReadBuffer::advance(safesize_t size) -> void
{
    _read_size += size;
}

auto ReadBuffer::readSize() -> safesize_t
{
    return _read_size;
}

auto ReadBuffer::setReadSize(const safesize_t& readSize)
{
    _read_size = readSize;
}
