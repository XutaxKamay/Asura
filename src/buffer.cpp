#include "pch.h"

#include "buffer.h"

using namespace XKLib;

auto XKLib::get_variable_type_str(const TypeSize typeSize) -> std::string
{
    static const std::string strings[] = { "safesize (32 bits)",
                                           "8 bits signed",
                                           "16 bits signed",
                                           "32 bits signed",
                                           "64 bits signed",
                                           "8 bits unsigned",
                                           "16 bits unsigned",
                                           "32 bits unsigned",
                                           "64 bits unsigned",
                                           "array",
                                           "float",
                                           "double" };

    if (typeSize < strings->size())
    {
        return "unknown";
    }

    return strings[typeSize];
}

Buffer::Buffer(const std::size_t maxSize)
 : _max_size(maxSize),
   _allocated(true)
{
    _data = alloc<data_t>(_max_size);
}

Buffer::Buffer(const data_t data, const std::size_t maxSize)
 : _max_size(maxSize)
{
    if (data)
    {
        _data      = data;
        _allocated = false;
    }
    else if (_max_size)
    {
        _data      = alloc<data_t>(_max_size);
        _allocated = true;
    }
    else
    {
        _allocated = false;
        /* vector maybe */
    }
}

Buffer::~Buffer()
{
    // Free data.
    if (_allocated)
    {
        free(_data);
    }
}

auto Buffer::operator[](const std::size_t size) const -> const auto&
{
    if (not _data and size >= _max_size)
    {
        XKLIB_EXCEPTION("Out of bounds.");
    }

    return *shift<data_t>(size);
}

auto Buffer::data() const -> data_t
{
    return _data;
}

auto Buffer::maxSize() const -> std::size_t
{
    return _max_size;
}

auto Buffer::toBytes() const -> bytes_t
{
    bytes_t bs(_max_size);
    std::copy(this->_data, this->_data + _max_size, bs.begin());

    return bs;
}

auto Buffer::operator[](const std::size_t size) -> auto&
{
    if (not _data and size >= _max_size)
    {
        XKLIB_EXCEPTION("Out of bounds.");
    }

    return *shift<data_t>(size);
}
