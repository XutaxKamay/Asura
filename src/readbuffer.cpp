#include "pch.h"

#include "readbuffer.h"

using namespace XKLib;

ReadBuffer::ReadBuffer(data_t data,
                       std::size_t maxSize,
                       std::size_t readSize)
 : Buffer(data, maxSize),
   _read_size(readSize)
{
}

auto ReadBuffer::readSize() const -> const std::size_t&
{
    return _read_size;
}

auto ReadBuffer::reset() -> void
{
    _read_size = 0;
}

auto ReadBuffer::advance(std::size_t size) -> void
{
    _read_size += size;
}

auto ReadBuffer::readSize() -> std::size_t&
{
    return _read_size;
}
