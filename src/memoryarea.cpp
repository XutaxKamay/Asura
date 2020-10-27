#include "memoryarea.h"
#include "memoryexception.h"

#ifdef WINDOWS
    #include "windows.h"
#endif

using namespace XLib;

auto MemoryArea::Protection::toOwn(int flags) -> memory_protection_flags_t
{
#ifdef WINDOWS
    memory_protection_flags_t own_flags;

    switch (flags)
    {
        case PAGE_EXECUTE:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::EXECUTE);
            break;
        }
        case PAGE_EXECUTE_READ:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::EXECUTE
              | memory_protection_flags_t::READ);
            break;
        }
        case PAGE_EXECUTE_READWRITE:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::EXECUTE
              | memory_protection_flags_t::READ
              | memory_protection_flags_t::WRITE);
            break;
        }
        case PAGE_READONLY:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::READ);
            break;
        }
        case PAGE_READWRITE:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::READ
              | memory_protection_flags_t::WRITE);
            break;
        }
        case PAGE_EXECUTE_WRITECOPY:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::EXECUTE
              | memory_protection_flags_t::WRITE);
            break;
        }
        case PAGE_WRITECOPY:
        {
            own_flags = view_as<memory_protection_flags_t>(
              memory_protection_flags_t::WRITE);
            break;
        }
        default:
        {
            own_flags = memory_protection_flags_t::NONE;
            break;
        }
    }

    return own_flags;

#else
    return view_as<memory_protection_flags_t>(
      flags
      & (memory_protection_flags_t::EXECUTE
         | memory_protection_flags_t::READ
         | memory_protection_flags_t::WRITE));
#endif
}

auto MemoryArea::Protection::toOS(memory_protection_flags_t flags) -> int
{
#ifdef WINDOWS
    int os_flags;

    switch (view_as<int>(flags))
    {
        case memory_protection_flags_t::EXECUTE:
        {
            os_flags = PAGE_EXECUTE;
            break;
        }
        case memory_protection_flags_t::EXECUTE
          | memory_protection_flags_t::READ:
        {
            os_flags = PAGE_EXECUTE_READ;
            break;
        }
        case memory_protection_flags_t::EXECUTE
          | memory_protection_flags_t::READ
          | memory_protection_flags_t::WRITE:
        {
            os_flags = PAGE_EXECUTE_READWRITE;
            break;
        }
        case memory_protection_flags_t::READ:
        {
            os_flags = PAGE_READONLY;
            break;
        }
        case memory_protection_flags_t::READ
          | memory_protection_flags_t::WRITE:
        {
            os_flags = PAGE_READWRITE;
            break;
        }
        case memory_protection_flags_t::EXECUTE
          | memory_protection_flags_t::WRITE:
        {
            os_flags = PAGE_EXECUTE_WRITECOPY;
            break;
        }
        case memory_protection_flags_t::WRITE:
        {
            os_flags = PAGE_WRITECOPY;
            break;
        }
        default:
        {
            os_flags = 0;
            break;
        }
    }

    return os_flags;
#else
    return flags;
#endif
}

auto MemoryArea::setAddress(ptr_t address) -> void
{
    _address = address;
}

auto MemoryArea::setSize(size_t size) -> void
{
    _size = size;
}

template <typename T>
auto MemoryArea::begin() -> T
{
    return view_as<T>(_address);
}

template <typename T>
auto MemoryArea::end() -> T
{
    return view_as<T>(view_as<uintptr_t>(_address) + _size);
}

template <typename T>
auto MemoryArea::size() -> T
{
    return view_as<T>(end<size_t>() - begin<size_t>());
}

template auto MemoryArea::size<size_t>() -> size_t;
