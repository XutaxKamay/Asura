#ifndef XKLIB_OFFSET_H
#define XKLIB_OFFSET_H

#include "memoryutils.h"

namespace XKLib
{
    class Offset
    {
      public:
        template <std::size_t O, typename T>
        auto member_at() const -> T
        {
            return view_as<T>(view_as<std::uintptr_t>(this) + O);
        }

        template <typename T, typename... A>
        auto call_at(const auto addr, A... args) const
        {
#ifndef WIN32
            return view_as<T (*)(decltype(this), A...)>(addr)(this,
                                                              args...);
#else
            return view_as<T(__thiscall*)(decltype(this), A...)>(
              addr)(this, args...);
#endif
        }
    };
}

#endif
