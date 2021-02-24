#include "buffer.h"

using namespace XLib;

std::string XLib::get_variable_type_str(typesize_t typeSize)
{
    switch (typeSize)
    {
        case (type_safesize):
            return "safesize (32 bits)";
        case (type_8s):
            return "8 bits signed";
        case (type_16s):
            return "16 bits signed";
        case (type_32s):
            return "32 bits signed";
        case (type_64s):
            return "64 bits signed";
        case (type_8us):
            return "8 bits unsigned";
        case (type_16us):
            return "16 bits unsigned";
        case (type_32us):
            return "32 bits unsigned";
        case (type_64us):
            return "64 bits unsigned";
        case (type_array):
            return "array";
        case (type_float):
            return "float";
        case (type_double):
            return "double";
        default:
            return "unknown";
    }
}

Buffer::Buffer(data_t data, safesize_t maxSize)
 : _data(nullptr), _max_size(maxSize), _allocated(false)
{
    if (data)
    {
        _data = data;
    }
    else if (_max_size)
    {
        _data      = new byte_t[_max_size];
        _allocated = true;
    }
    else
    {
        /* vector maybe */
    }
}

Buffer::~Buffer()
{
    // Free data.
    if (_allocated)
        free(_data);
}

auto& Buffer::operator[](safesize_t size)
{
    if (!_data && size >= _max_size)
    {
        throw XLIB_EXCEPTION("Out of bounds.");
    }

    return *shift<data_t>(size);
}

auto Buffer::data() -> data_t
{
    return _data;
}

auto Buffer::setData(const data_t& data)
{
    _data = data;
}

auto Buffer::maxSize() -> safesize_t
{
    return _max_size;
}

auto Buffer::setMaxSize(const safesize_t& maxSize)
{
    _max_size = maxSize;
}

auto Buffer::toBytes() -> bytes_t
{
    bytes_t bs(_max_size);
    std::copy(this->_data, this->_data + _max_size, bs.begin());

    return bs;
}
