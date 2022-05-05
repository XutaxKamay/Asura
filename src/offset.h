#ifndef OFFSET_H
#define OFFSET_H

#include "memoryutils.h"

namespace XKLib
{
    class Offset
    {
      public:
        template <std::size_t offset, typename T>
        auto member_at() const -> T
        {
            return view_as<T>(view_as<std::uintptr_t>(this) + offset);
        }

        template <typename ret_T, typename... args_T>
        auto call_at(const auto addr, args_T... args) const
        {
#ifndef WIN32
            return view_as<ret_T (*)(decltype(this), args_T...)>(
              addr)(this, args...);
#else
            return view_as<ret_T(__thiscall*)(decltype(this), args_T...)>(
              addr)(this, args...);
#endif
        }
    };
}

#endif
