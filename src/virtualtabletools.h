#ifndef ASURA_VIRTUALTABLETOOLS_H
#define ASURA_VIRTUALTABLETOOLS_H

#include "buffer.h"
#include "process.h"

namespace Asura
{
    /**
     * A class can have multiple vtables,
     * so it is not always the first member of a class.
     */

    using vfunc_t                  = ptr_t;
    using vfuncs_t                 = std::vector<vfunc_t>;
    using pre_or_post_hook_vfunc_t = std::function<
      void(const ptr_t funcPtr, const ptr_t newFuncPtr)>;

    template <std::size_t N, typename T = ptr_t>
    constexpr inline auto view_vfunc_as(const ptr_t* const vptr)
    {
        return view_as<T>(vptr[N]);
    }

    template <typename T = ptr_t>
    constexpr inline auto view_vfunc_dyn_index_as(const ptr_t* const vptr,
                                                  const std::size_t index)
    {
        return view_as<T>(vptr[index]);
    }

    template <std::size_t N, typename T = void, typename... A>
    constexpr inline auto vfunc(const ptr_t* const vptr)
    {
#ifdef WINDOWS
        return view_vfunc_as<N, T(__thiscall*)(ptr_t, A...)>(vptr);
#else
        return view_vfunc_as<N, T (*)(ptr_t, A...)>(vptr);
#endif
    }

    template <typename T = void, typename... A>
    constexpr inline auto vfunc_dyn_index(ptr_t vptr,
                                          const std::size_t index)
    {
#ifdef WINDOWS
        return view_vfunc_dyn_index_as<T(__thiscall*)(ptr_t, A...)>(vptr,
                                                                    index);
#else
        return view_vfunc_dyn_index_as<T (*)(ptr_t, A...)>(vptr, index);
#endif
    }

    /**
     * Used for hooking
     */
    template <std::size_t N, typename T = ptr_t>
    constexpr inline auto vfunc_ptr(ptr_t* const vptr)
    {
        return view_as<T>(&vptr[N]);
    }

    template <typename T = ptr_t>
    constexpr inline auto vfunc_ptr_dyn_index(const ptr_t* const vptr,
                                              const std::size_t index)
    {
        return view_as<T>(&vptr[index]);
    }

    template <std::size_t N, typename T = void, typename... A>
    constexpr inline auto call_vfunc(const ptr_t classPtr,
                                     const ptr_t* const vptr,
                                     A... args)
    {
        return vfunc<N, T, A...>(vptr)(classPtr, args...);
    }

    template <typename T = void, typename... A>
    constexpr inline auto call_vfunc_dyn_index(const ptr_t classPtr,
                                               const ptr_t* const vptr,
                                               std::size_t index,
                                               A... args)
    {
        return vfunc_dyn_index<T, A...>(vptr, index)(classPtr, args...);
    }

    template <class C>
    class VirtualTable : public C
    {
      public:
        template <std::size_t N, typename T = void, typename... A>
        constexpr inline auto callVFunc(const ptr_t* const vptr,
                                        A... args)
        {
            return call_vfunc<N, T, A...>(this, vptr, args...);
        }

        template <typename T = void, typename... A>
        constexpr inline auto callVfuncDynIndex(const ptr_t* const vptr,
                                                const std::size_t index,
                                                A... args)
        {
            return call_vfunc_dyn_index<T, A...>(vptr, index)(this,
                                                              vptr,
                                                              index,
                                                              args...);
        }
    };

    template <std::size_t N>
    inline auto hook_vfunc(ptr_t* const vptr,
                           const auto newFuncPtr,
                           const pre_or_post_hook_vfunc_t pre  = nullptr,
                           const pre_or_post_hook_vfunc_t post = nullptr)
    {
        const auto funcPtr = vfunc_ptr<N, ptr_t* const>(vptr);

        mapf_t flags;
        std::shared_ptr<ProcessMemoryArea> area;

        if (funcPtr and *funcPtr and newFuncPtr)
        {
            if (pre)
            {
                pre(view_as<ptr_t>(funcPtr), view_as<ptr_t>(newFuncPtr));
            }
            else
            {
                const auto process = Process::self();
                const auto& mmap   = process.mmap();

                area = mmap.search(funcPtr);

                flags = area->protectionFlags().cachedValue();

                area->protectionFlags() = MemoryArea::ProtectionFlags::RWX;
            }

            /* __builtin_trap(); */

            *funcPtr = view_as<ptr_t>(newFuncPtr);

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

}

#endif
