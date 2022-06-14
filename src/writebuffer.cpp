#include "pch.h"

#include "writebuffer.h"

using namespace XKLib;

WriteBuffer::WriteBuffer(data_t data,
                         std::size_t maxSize,
                         std::size_t writeSize)
 : Buffer(data, maxSize),
   _written_size(writeSize)
{
}

auto WriteBuffer::writeSize() const -> const std::size_t&
{
    return _written_size;
}

auto WriteBuffer::toBytes() const -> bytes_t
{
    bytes_t bs(_written_size);
    std::copy(this->data(), this->data() + _written_size, bs.begin());

    return bs;
}

auto WriteBuffer::addType(TypeSize typeSize) -> void
{
    addData(&typeSize, view_as<std::size_t>(sizeof(typeSize)));
}

auto WriteBuffer::reset() -> void
{
    _written_size = 0;
}

auto WriteBuffer::addData(ptr_t data, std::size_t size) -> void
{
    if (_written_size >= maxSize())
    {
        XKLIB_EXCEPTION("Filled buffer");
    }

    std::copy(view_as<data_t>(data),
              view_as<data_t>(view_as<std::uintptr_t>(data)
                              + view_as<std::size_t>(size)),
              view_as<data_t>(shift(_written_size)));

    advance(size);
}

auto WriteBuffer::advance(std::size_t size) -> void
{
    _written_size += size;
}

auto WriteBuffer::writeSize() -> std::size_t&
{
    return _written_size;
}
