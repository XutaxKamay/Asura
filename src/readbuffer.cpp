#include "readbuffer.h"

using namespace XLib;

ReadBuffer::ReadBuffer(data_t data,
                       safesize_t maxSize,
                       safesize_t readSize)
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

auto ReadBuffer::setReadSize(XLib::safesize_t readSize)
{
    _read_size = readSize;
}
