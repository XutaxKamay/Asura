#include "memoryarea.h"
#include "exception.h"

#ifdef WINDOWS
    #include "windows.h"
#endif

using namespace XLib;

auto MemoryArea::ProtectionFlags::ToOwn(mapf_t flags) -> mapf_t
{
#ifdef WINDOWS
    mapf_t own_flags;

    switch (flags)
    {
        case PAGE_EXECUTE:
        {
            own_flags = MemoryArea::ProtectionFlags::EXECUTE;
            break;
        }
        case PAGE_EXECUTE_READ:
        {
            own_flags = MemoryArea::ProtectionFlags::EXECUTE
                        | MemoryArea::ProtectionFlags::READ;
            break;
        }
        case PAGE_EXECUTE_READWRITE:
        {
            own_flags = MemoryArea::ProtectionFlags::EXECUTE
                        | MemoryArea::ProtectionFlags::READ
                        | MemoryArea::ProtectionFlags::WRITE;
            break;
        }
        case PAGE_READONLY:
        {
            own_flags = MemoryArea::ProtectionFlags::READ;
            break;
        }
        case PAGE_READWRITE:
        {
            own_flags = MemoryArea::ProtectionFlags::READ
                        | MemoryArea::ProtectionFlags::WRITE;
            break;
        }
        case PAGE_EXECUTE_WRITECOPY:
        {
            own_flags = MemoryArea::ProtectionFlags::EXECUTE
                        | MemoryArea::ProtectionFlags::WRITE;
            break;
        }
        case PAGE_WRITECOPY:
        {
            own_flags = MemoryArea::ProtectionFlags::WRITE;
            break;
        }
        default:
        {
            own_flags = MemoryArea::ProtectionFlags::NONE;
            break;
        }
    }

    return own_flags;

#else
    return flags
           & (MemoryArea::ProtectionFlags::EXECUTE
              | MemoryArea::ProtectionFlags::READ
              | MemoryArea::ProtectionFlags::WRITE);
#endif
}

auto MemoryArea::ProtectionFlags::ToOS(mapf_t flags) -> mapf_t
{
#ifdef WINDOWS
    int os_flags;

    switch (view_as<int>(flags))
    {
        case MemoryArea::ProtectionFlags::EXECUTE:
        {
            os_flags = PAGE_EXECUTE;
            break;
        }
        case MemoryArea::ProtectionFlags::EXECUTE
          | MemoryArea::ProtectionFlags::READ:
        {
            os_flags = PAGE_EXECUTE_READ;
            break;
        }
        case MemoryArea::ProtectionFlags::EXECUTE
          | MemoryArea::ProtectionFlags::READ
          | MemoryArea::ProtectionFlags::WRITE:
        {
            os_flags = PAGE_EXECUTE_READWRITE;
            break;
        }
        case MemoryArea::ProtectionFlags::READ:
        {
            os_flags = PAGE_READONLY;
            break;
        }
        case MemoryArea::ProtectionFlags::READ
          | MemoryArea::ProtectionFlags::WRITE:
        {
            os_flags = PAGE_READWRITE;
            break;
        }
        case MemoryArea::ProtectionFlags::EXECUTE
          | MemoryArea::ProtectionFlags::WRITE:
        {
            os_flags = PAGE_EXECUTE_WRITECOPY;
            break;
        }
        case MemoryArea::ProtectionFlags::WRITE:
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

auto XLib::MemoryArea::setName(const std::string& name) -> void
{
    _name = name;
}

auto MemoryArea::operator==(MemoryArea& area) -> bool
{
    return begin() == area.begin() && end() == area.end();
}

auto XLib::MemoryArea::name() -> const std::string&
{
    return _name;
}
