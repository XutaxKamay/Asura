#ifndef XKLIB_DETOURX86_H
#define XKLIB_DETOURX86_H

#include "process.h"

#ifdef WINDOWS
enum CallingConventions
{
    cc_fastcall,
    cc_stdcall,
    cc_cdecl
};
#endif

namespace XKLib
{
    namespace X86_JMP
    {
        constexpr byte_t REL_INST = 0xE9;

        auto override_rel32(
          const auto fromPtr,
          const auto to,
          const std::function<void(decltype(fromPtr), decltype(to))> beforeOverride = nullptr,
          const std::function<void(decltype(fromPtr), decltype(to))> afterOverride = nullptr)
        {
            const auto old_func = view_as<std::uintptr_t>(
                                    *view_as<std::int32_t*>(
                                      view_as<std::uintptr_t>(fromPtr)
                                      + 1))
                                  + (view_as<std::uintptr_t>(fromPtr)
                                     + 5);

            mapf_t flags;
            std::shared_ptr<ProcessMemoryArea> area;

            if (beforeOverride)
            {
                beforeOverride(fromPtr, to);
            }
            else
            {
                const auto process = Process::self();
                const auto& mmap   = process.mmap();

                area = mmap.search(fromPtr);

                flags = area->protectionFlags().cachedValue();

                area->protectionFlags() = MemoryArea::ProtectionFlags::RWX;
            }

            *view_as<std::int32_t*>(view_as<std::uintptr_t>(fromPtr) + 1)
              = view_as<std::int32_t>(to)
                - (view_as<std::int32_t>(fromPtr) + 5);

            if (afterOverride)
            {
                afterOverride(fromPtr, to);
            }
            else
            {
                area->protectionFlags() = flags;
            }

            return old_func;
        }
    };

    namespace X86_CALL
    {
        constexpr byte_t REL_INST = 0xE8;

        auto override_rel32(
          const auto fromPtr,
          const auto to,
          const std::function<void(decltype(fromPtr), decltype(to))> beforeOverride = nullptr,
          const std::function<void(decltype(fromPtr), decltype(to))> afterOverride = nullptr)
        {
            return X86_JMP::override_rel32(fromPtr,
                                           to,
                                           beforeOverride,
                                           afterOverride);
        }
    };

        /**
         * TODO:
         * here something i didn't finish yet but yea, ill use this upside
         * instead'
         */
#ifdef _WIN32
    template <CallingConventions C, typename T, typename... A>
#else
    template <typename T, typename... A>
#endif
    /**
     * This class permits to hook any functions inside the current
     * process. Detour is a method to hook functions. It works generally
     * by placing a JMP instruction on the start of the function. This
     * ones works by copying a small portion of opcodes that the JMP
     * instruction override, disassemble them and search the closest
     * address to the function address in order to allocate memory, so we
     * can use a relative JMP instruction. If the disassembled
     * instructions contains addresses that needs relocation since they're
     * relative most of the time, it will automatically patch them by
     * checking if it's pointing to the valid address and memory or not.
     */
    class TraditionalDetourX86
    {
        /**
         * This is a manager in order to find for each detours
         * the closest memory area possible we can use in order to jmp
         * and callback the instructions we've overwritten.
         */
        class Fragment
        {
          public:
            std::uintptr_t address;
            std::size_t size;
        };

        using HandleFragment_t = std::shared_ptr<Fragment>;

        class FragmentsArea
        {
          public:
            std::shared_ptr<ProcessMemoryArea> process_memory_area;
            std::vector<std::shared_ptr<Fragment>> _fragments;
        };

        class FragmentManager
        {
          public:
            FragmentManager();

            auto newFragment(bytes_t data, ptr_t originalFunc)
              -> HandleFragment_t;

            auto wipeFragment(HandleFragment_t HandleFragment);

          private:
            std::vector<std::shared_ptr<FragmentsArea>> _fragments_area;
        };

      private:
        /* This case is only for windows 32 bits program */
#ifdef _WIN32
        static constexpr auto GenerateCallBackFuncType();
        static constexpr auto GenerateNewFuncType();

      public:
        using cbfunc_t = typename decltype(GenerateCallBackFuncType())::type;
        using func_t = typename decltype(GenerateNewFuncType())::type;

#else /* Otherwise it should be always the same convention */
        using cbfunc_t = T (*)(A...);
        using func_t   = cbfunc_t;
#endif
      public:
        TraditionalDetourX86(cbfunc_t originalFunc, ptr_t newFunc);

      private:
        cbfunc_t _callback_func;
        func_t _new_func;
        func_t _original_func;
        HandleFragment_t _handle_fragment;
    };

/**
 * On windows 32 bits programs, the calling convention needs to be
 * specified in order to call back the original function when
 * detoured. It is not needed on others architectures/os because the
 * calling conventions are always the same (64 bits is fastcall on
 * Windows, same for Linux), and 32 bits on Linux the parameters are
 * always pushed to the stack.
 */
#ifdef _WIN32
    template <CallingConventions C, typename T, typename... A>
    constexpr auto TraditionalDetourX86<C, T, A...>::
      GenerateCallBackFuncType()
    {
        if constexpr (C == cc_fastcall)
        {
            /* thisptr - EDX, stack params */
            return type_wrapper<T(__thiscall*)(ptr_t, A...)>;
        }
        else if constexpr (C == cc_stdcall)
        {
            return type_wrapper<T(__stdcall*)(A...)>;
        }
        else
        {
            return type_wrapper<T (*)(A...)>;
        }
    }

    template <CallingConventions C, typename T, typename... A>
    constexpr auto TraditionalDetourX86<C, T, A...>::GenerateNewFuncType()
    {
        if constexpr (C == cc_fastcall)
        {
            /* EDX, ECX, stack params */
            return type_wrapper<T(__fastcall*)(ptr_t, ptr_t, A...)>;
        }
        else if constexpr (C == cc_stdcall)
        {
            return type_wrapper<T(__stdcall*)(A...)>;
        }
        else
        {
            return type_wrapper<T (*)(A...)>;
        }
    }
#endif
}

#endif
