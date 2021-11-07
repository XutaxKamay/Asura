#ifndef OFFSET_H
#define OFFSET_H

#include "memoryutils.h"

namespace XKLib
{
    class Offset
    {
        public:
            template <size_t offset, typename T>
            auto member_at() -> T
            {
                return view_as<T>(view_as<uintptr_t>(this) + offset);
            }

            template <typename ret_T, typename... args_T>
            auto call_at(auto addr, args_T... args)
            {
#ifndef WIN32
                return view_as<ret_T (*)(ptr_t, args_T...)>(
                  addr)(this, args...);
#else
                return view_as<ret_T(__thiscall*)(ptr_t, args_T...)>(
                  addr)(this, args...);
#endif
            }
    };
}

#endif
