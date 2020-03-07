#include "buffer.h"

using namespace XLib;

template <typename T>
constexpr inline auto XLib::alloc(safesize_t size)
{
    return reinterpret_cast<T>(::operator new(size));
}

template <typename T>
constexpr inline void XLib::free(T& pBuf)
{
    ::operator delete(reinterpret_cast<ptr_t>(pBuf));
}

template <typesize_t type>
constexpr inline auto _gvt()

{
    if constexpr (type == type_safesize)
        return _type<safesize_t>;
    else if constexpr (type == type_8)
        return _type<byte_t>;
    else if constexpr (type == type_16)
        return _type<uint16_t>;
    else if constexpr (type == type_32)
        return _type<uint32_t>;
    else if constexpr (type == type_64)
        return _type<uint64_t>;
    else if constexpr (type == type_array)
        return _type<array_t>;
    else if constexpr (type == type_float)
        return _type<float>;
    else if constexpr (type == type_double)
        return _type<double>;
    else
        return _type<void>;
}

inline std::string gvtStr(typesize_t typeSize)
{
    if (typeSize == type_safesize)
        return "safesize (32 bits)";
    else if (typeSize == type_8)
        return "8 bits";
    else if (typeSize == type_16)
        return "16 bits";
    else if (typeSize == type_32)
        return "32 bits";
    else if (typeSize == type_64)
        return "64 bits";
    else if (typeSize == type_array)
        return "array";
    else if (typeSize == type_float)
        return "float";
    else if (typeSize == type_double)
        return "double";
    else
        return "unknown";
}

template <safesize_t max_size>
Buffer<max_size>::Buffer() : _maxSize(max_size), _allocated(max_size != 0)
{
    // Allocate with the maximum size for performance.
    if constexpr (max_size != 0)
        _pData = alloc<decltype(_pData)>(max_size);
}

template <safesize_t max_size>
Buffer<max_size>::Buffer(array_t pData, bool allocated, safesize_t maxSize) :
    _pData(pData), _maxSize(maxSize), _allocated(allocated)
{}

template <safesize_t max_size>
Buffer<max_size>::~Buffer()
{
    // Free data.
    if (_allocated)
        free(_pData);
}

template <safesize_t max_size>
constexpr auto& Buffer<max_size>::operator[](safesize_t size)
{
    return *shift<byte_t*>(size);
}

template <safesize_t max_size>
constexpr inline array_t Buffer<max_size>::pData() const
{
    return _pData;
}

template <safesize_t max_size>
constexpr inline void Buffer<max_size>::setPData(const array_t& pData)
{
    _pData = pData;
}

template <safesize_t max_size>
constexpr inline safesize_t Buffer<max_size>::maxSize() const
{
    return _maxSize;
}

template <safesize_t max_size>
constexpr inline void Buffer<max_size>::setMaxSize(const safesize_t& maxSize)
{
    _maxSize = maxSize;
}
