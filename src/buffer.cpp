#include "pch.h"

#ifdef WINDOWS
    #include <windows.h>

    #include <dbghelp.h>
    #include <psapi.h>
    #include <tlhelp32.h>
#endif

#include "buffer.h"

using namespace XKLib;

auto XKLib::get_variable_type_str(typesize_t typeSize) -> std::string
{
    static std::string strings[] = { "safesize (32 bits)",
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

Buffer::Buffer(std::size_t maxSize) : _max_size(maxSize), _allocated(true)
{
    _data = alloc<data_t>(_max_size);
}

Buffer::Buffer(data_t data, std::size_t maxSize) : _max_size(maxSize)
{
    if (data)
    {
        _data = data;
    }
    else if (_max_size)
    {
        _data      = alloc<data_t>(_max_size);
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

auto Buffer::operator[](std::size_t size) -> auto&
{
    if (!_data && size >= _max_size)
    {
        XKLIB_EXCEPTION("Out of bounds.");
    }

    return *shift<data_t>(size);
}

auto Buffer::data() -> data_t
{
    return _data;
}

auto Buffer::setData(XKLib::data_t data)
{
    _data = data;
}

auto Buffer::maxSize() -> std::size_t
{
    return _max_size;
}

auto Buffer::setMaxSize(std::size_t maxSize)
{
    _max_size = maxSize;
}

auto Buffer::toBytes() -> bytes_t
{
    bytes_t bs(_max_size);
    std::copy(this->_data, this->_data + _max_size, bs.begin());

    return bs;
}
