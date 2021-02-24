#include "writebuffer.h"

using namespace XLib;

WriteBuffer::WriteBuffer(data_t data,
                         safesize_t writeSize,
                         safesize_t maxSize)
 : Buffer(data, maxSize), _written_size(writeSize)
{
}

auto WriteBuffer::addType(typesize_t typeSize) -> void
{
    addData(&typeSize, view_as<safesize_t>(sizeof(typeSize)));
}

auto WriteBuffer::reset() -> void
{
    _written_size = 0;
}

auto WriteBuffer::addData(ptr_t data, safesize_t size) -> void
{
    if (_written_size >= maxSize())
    {
        throw XLIB_EXCEPTION("Filled buffer");
    }

    std::copy(view_as<data_t>(data),
              view_as<data_t>(data) + view_as<size_t>(size),
              view_as<data_t>(shift(_written_size)));

    advance(size);
}

auto WriteBuffer::advance(safesize_t size) -> void
{
    _written_size += size;
}

auto WriteBuffer::writeSize() -> safesize_t
{
    return _written_size;
}

auto WriteBuffer::setWriteSize(const safesize_t& writeSize) -> void
{
    _written_size = writeSize;
}

auto WriteBuffer::toBytes() -> bytes_t
{
    bytes_t bs(_written_size);
    std::copy(this->_data, this->_data + _written_size, bs.begin());

    return bs;
}
