#ifndef VIRTUALTABLETOOLS_H
#define VIRTUALTABLETOOLS_H

#include <functional>

#include "buffer.h"
#include "process.h"

namespace XKLib
{
    /**
     * A class can have multiple vtables,
     * so it is not always the first member of a class.
     */

    using vfunc_t                  = ptr_t;
    using vfuncs_t                 = std::vector<vfunc_t>;
    using pre_or_post_hook_vfunc_t = std::function<void(ptr_t funcPtr,
                                                        ptr_t newFuncPtr)>;

    template <size_t index_T, typename T = ptr_t>
    constexpr inline auto view_vfunc_as(ptr_t* vptr)
    {
        return view_as<T>(vptr[index_T]);
    }

    template <typename T = ptr_t>
    constexpr inline auto view_vfunc_dyn_index_as(ptr_t* vptr,
                                                  size_t index)
    {
        return view_as<T>(vptr[index]);
    }

    template <size_t index_T, typename ret_type_T = void, typename... args_T>
    constexpr inline auto vfunc(ptr_t* vptr)
    {
#ifdef WINDOWS
        return view_vfunc_as<index_T,
                             ret_type_T(__thiscall*)(ptr_t, args_T...)>(
          vptr);
#else
        return view_vfunc_as<index_T, ret_type_T (*)(ptr_t, args_T...)>(
          vptr);
#endif
    }

    template <typename ret_type_T = void, typename... args_T>
    constexpr inline auto vfunc_dyn_index(ptr_t vptr, size_t index)
    {
#ifdef WINDOWS
        return view_vfunc_dyn_index_as<ret_type_T(
          __thiscall*)(ptr_t, args_T...)>(vptr, index);
#else
        return view_vfunc_dyn_index_as<ret_type_T (*)(ptr_t, args_T...)>(
          vptr,
          index);
#endif
    }

    /**
     * Used for hooking
     */
    template <size_t index_T, typename T = ptr_t>
    constexpr inline auto vfunc_ptr(ptr_t* vptr)
    {
        return view_as<T>(&vptr[index_T]);
    }

    template <typename T = ptr_t>
    constexpr inline auto vfunc_ptr_dyn_index(ptr_t* vptr, size_t index)
    {
        return view_as<T>(&vptr[index]);
    }

    template <size_t index_T, typename ret_type_T = void, typename... args_T>
    constexpr inline auto call_vfunc(ptr_t classPtr,
                                     ptr_t* vptr,
                                     args_T... args)
    {
        return vfunc<index_T, ret_type_T, args_T...>(vptr)(classPtr,
                                                           args...);
    }

    template <typename ret_type_T = void, typename... args_T>
    constexpr inline auto call_vfunc_dyn_index(ptr_t classPtr,
                                               ptr_t* vptr,
                                               size_t index,
                                               args_T... args)
    {
        return vfunc_dyn_index<ret_type_T, args_T...>(vptr,
                                                      index)(classPtr,
                                                             args...);
    }

    template <class T>
    class VirtualTable : public T
    {
      public:
        template <size_t index_T,
                  typename ret_type_T = void,
                  typename... args_T>
        constexpr inline auto callVFunc(ptr_t* vptr, args_T... args)
        {
            return call_vfunc<index_T, ret_type_T, args_T...>(this,
                                                              vptr,
                                                              args...);
        }

        template <typename ret_type_T = void, typename... args_T>
        constexpr inline auto callVfuncDynIndex(ptr_t* vptr,
                                                size_t index,
                                                args_T... args)
        {
            return call_vfunc_dyn_index<ret_type_T, args_T...>(
              vptr,
              index)(this, vptr, index, args...);
        }
    };

    template <size_t index_T, typename T2 = ptr_t>
    inline auto hook_vfunc(ptr_t* vptr,
                           T2 newFuncPtr,
                           pre_or_post_hook_vfunc_t pre  = nullptr,
                           pre_or_post_hook_vfunc_t post = nullptr)
    {
        auto funcPtr = vfunc_ptr<index_T, T2*>(vptr);

        mapf_t flags;
        std::shared_ptr<ProcessMemoryArea> area;

        if (funcPtr && *funcPtr && newFuncPtr)
        {
            if (pre)
            {
                pre(view_as<ptr_t>(funcPtr), view_as<ptr_t>(newFuncPtr));
            }
            else
            {
                auto mmap = Process::self().mmap();

                area = mmap.search(funcPtr);

                flags = area->protectionFlags().cachedValue();

                area->protectionFlags() = MemoryArea::ProtectionFlags::RWX;
            }

            /* __builtin_trap(); */

            *funcPtr = newFuncPtr;

            if (post)
            {
                post(view_as<ptr_t>(funcPtr), view_as<ptr_t>(newFuncPtr));
            }
            else
            {
                area->protectionFlags() = flags;
            }

            return true;
        }

        return false;
    }

} // namespace XKLib

#endif // VIRTUALTABLETOOLS_H
