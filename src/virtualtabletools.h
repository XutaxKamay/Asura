#ifndef VIRTUALTABLETOOLS_H
#define VIRTUALTABLETOOLS_H

#include "buffer.h"
#include "types.h"
#include <functional>

namespace XLib
{
    using vfunc_t  = ptr_t;
    using vfuncs_t = std::vector<vfunc_t>;

    template <typename T = ptr_t>
    constexpr auto vtable(T classPtr)
    {
        return *view_as<ptr_t**>(classPtr);
    }

    template <safesize_t index_T, typename T = ptr_t>
    constexpr inline auto view_vfunc_as(ptr_t classPtr)
    {
        return view_as<T>(vtable(classPtr)[index_T]);
    }

    template <typename T = ptr_t>
    constexpr inline auto view_vfunc_dyn_index_as(ptr_t classPtr,
                                                  safesize_t index)
    {
        return view_as<T>(vtable(classPtr)[index]);
    }

    template <safesize_t index_T, typename T = ptr_t>
    constexpr inline auto vfunc_ptr(ptr_t classPtr)
    {
        return view_as<T>(&vtable(classPtr)[index_T]);
    }

    template <typename T = ptr_t>
    constexpr inline auto vfunc_ptr_dyn_index(ptr_t classPtr,
                                              safesize_t index)
    {
        return view_as<T>(&vtable(classPtr)[index]);
    }

    template <safesize_t index_T,
              typename ret_type_T = void,
              typename... args_T>
    constexpr inline auto vfunc(ptr_t classPtr)
    {
#ifdef WINDOWS
        return view_vfunc_as<index_T,
                             ret_type_T(__thiscall*)(ptr_t, args_T...)>(
          classPtr);
#else
        return view_vfunc_as<index_T, ret_type_T (*)(ptr_t, args_T...)>(
          classPtr);
#endif
    }

    template <typename ret_type_T = void, typename... args_T>
    constexpr inline auto vfunc_dyn_index(ptr_t classPtr,
                                          safesize_t index)
    {
#ifdef WINDOWS
        return view_vfunc_dyn_index_as<ret_type_T(
          __thiscall*)(ptr_t, args_T...)>(classPtr, index);
#else
        return view_vfunc_dyn_index_as<ret_type_T (*)(ptr_t, args_T...)>(
          classPtr,
          index);
#endif
    }

    template <safesize_t index_T,
              typename ret_type_T = void,
              typename... args_T>
    constexpr inline auto call_vfunc(ptr_t classPtr, args_T... args)
    {
        return vfunc<index_T, ret_type_T, args_T...>(classPtr)(classPtr,
                                                               args...);
    }

    template <typename ret_type_T = void, typename... args_T>
    constexpr inline auto call_vfunc_dyn_index(ptr_t classPtr,
                                               safesize_t index,
                                               args_T... args)
    {
        return vfunc_dyn_index<ret_type_T, args_T...>(classPtr,
                                                      index)(classPtr,
                                                             args...);
    }

    template <class T>
    constexpr inline auto construct_vtable_from_vfuncs(
      const vfuncs_t& vfuncs,
      safesize_t classSize = 0)
    {
        using class_ptr_t = std::unique_ptr<T*>;

        auto classPtr = class_ptr_t(classSize == 0 ? new T :
                                                     alloc(classSize));

        auto vfuncsInsideVTable = view_as<ptr_t*>(
          alloc(vfuncs.size() * sizeof(vfunc_t)));

        /**
         * Setup hidden _vptr member
         */
        *view_as<ptr_t*>(classPtr) = vfuncsInsideVTable;

        for (auto&& vfunc : vfuncs)
        {
            *vfuncsInsideVTable = vfunc;

            vfuncsInsideVTable++;
        }

        return classPtr;
    }

    template <class T>
    class VirtualTable : public T
    {
        using pre_or_post_hook_func_t = std::function<
          void(ptr_t funcPtr, ptr_t newFuncPtr)>;

      public:
        template <safesize_t index_T,
                  typename ret_type_T = void,
                  typename... args_T>
        constexpr inline auto callVFunc(args_T... args)
        {
            return call_vfunc<index_T, ret_type_T, args_T...>(this,
                                                              args...);
        }

        template <typename ret_type_T = void, typename... args_T>
        constexpr inline auto callVFunc(safesize_t index, args_T... args)
        {
            return call_vfunc_dyn_index<ret_type_T, args_T...>(this,
                                                               index,
                                                               args...);
        }

        template <safesize_t index_T,
                  typename ret_type_T = void,
                  typename... args_T>
        constexpr inline auto VFunc()
        {
            return vfunc<index_T, ret_type_T, args_T...>(this);
        }

        template <typename ret_type_T = void, typename... args_T>
        constexpr inline auto VFuncDynIndex(safesize_t index)
        {
            return vfunc_dyn_index<ret_type_T, args_T...>(this, index);
        }

        template <safesize_t index_T, typename T2 = ptr_t>
        constexpr inline auto ViewVFuncAs()
        {
            return view_vfunc_as<index_T, T2>(this);
        }

        template <typename T2 = ptr_t>
        constexpr inline auto ViewVFuncDynIndexAs(safesize_t index)
        {
            return view_vfunc_dyn_index_as<T2>(this, index);
        }

        template <safesize_t index_T, typename T2 = ptr_t>
        constexpr inline auto VFuncPtr()
        {
            return vfunc_ptr<index_T, T2>(this);
        }

        template <typename T2 = ptr_t>
        constexpr inline auto VFuncPtrDynIndex(safesize_t index)
        {
            return vfunc_ptr_dyn_index<T2>(this, index);
        }

        template <safesize_t index_T, typename T2 = ptr_t>
        inline auto hook(T2 newFuncPtr,
                         pre_or_post_hook_func_t pre  = nullptr,
                         pre_or_post_hook_func_t post = nullptr)
        {
            auto funcPtr = VFuncPtr<index_T, T2*>();

            if (funcPtr && *funcPtr && newFuncPtr)
            {
                if (pre)
                {
                    pre(view_as<ptr_t>(funcPtr),
                        view_as<ptr_t>(newFuncPtr));
                }

                /* __builtin_trap(); */

                *funcPtr = newFuncPtr;

                if (post)
                {
                    post(view_as<ptr_t>(funcPtr),
                         view_as<ptr_t>(newFuncPtr));
                }

                return true;
            }

            return false;
        }

        template <typename T2 = ptr_t>
        constexpr inline auto _vptr()
        {
            return XLib::vtable<T2>(this);
        }

        auto listVFuncs() -> vfuncs_t;
        auto countVFuncs() -> safesize_t;
    };

    template <class T>
    auto VirtualTable<T>::countVFuncs() -> safesize_t
    {
        auto vtable = _vptr();

        safesize_t count = 0;

        while (vtable[count])
        {
            count++;
        }

        return count;
    }

    template <class T>
    auto VirtualTable<T>::listVFuncs() -> vfuncs_t
    {
        vfuncs_t vfuncs;

        auto vtable = _vptr();

        do
        {
            vfuncs.push_back(view_as<vfunc_t>(*vtable));
        }
        while (++vtable);

        return vfuncs;
    }
} // namespace XLib

#endif // VIRTUALTABLETOOLS_H
