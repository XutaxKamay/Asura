#ifndef DETOURX86_H
#define DETOURX86_H

#include "process.h"

#ifdef WINDOWS
enum calling_conventions_t
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

        template <typename T>
        auto override_rel32(
          T fromPtr,
          T to,
          std::function<void(T, T)> beforeOverride = nullptr,
          std::function<void(T, T)> afterOverride  = nullptr)
        {
            auto old_func = view_as<uintptr_t>(*view_as<int32_t*>(
                              view_as<uintptr_t>(fromPtr) + 1))
                            + (view_as<uintptr_t>(fromPtr) + 5);

            mapf_t flags;
            std::shared_ptr<ProcessMemoryArea> area;

            if (beforeOverride)
            {
                beforeOverride(fromPtr, to);
            }
            else
            {
                auto mmap = Process::self().mmap();

                area = mmap.search(fromPtr);

                flags = area->protectionFlags().cachedValue();

                area->protectionFlags() = MemoryArea::ProtectionFlags::RWX;
            }

            *view_as<int32_t*>(fromPtr + 1) = view_as<int32_t>(to)
                                              - (view_as<int32_t>(fromPtr)
                                                 + 5);

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

        template <typename T>
        auto override_rel32(
          T fromPtr,
          T to,
          std::function<void(T, T)> beforeOverride = nullptr,
          std::function<void(T, T)> afterOverride  = nullptr)
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
    template <calling_conventions_t calling_convention_T,
              typename ret_type_T,
              typename... args_T>
#else
    template <typename ret_type_T, typename... args_T>
#endif
    /**
     * @brief TraditionalDetourX86
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
         * the closest memory area possible we can use in order to jmp and
         * callback the instructions we've overwritten.
         */
        class Fragment
        {
          public:
            uintptr_t address;
            size_t size;
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
        /**
         * @brief generateCallBackFuncType
         *
         * @return auto
         */
        static constexpr auto GenerateCallBackFuncType();
        /**
         * @brief generateNewFuncType
         *
         * @return auto
         */
        static constexpr auto GenerateNewFuncType();

      public:
        using cbfunc_t = typename decltype(GenerateCallBackFuncType())::type;
        using func_t = typename decltype(GenerateNewFuncType())::type;

#else /* Otherwise it should be always the same convention */
        using cbfunc_t = ret_type_T (*)(args_T...);
        using func_t   = cbfunc_t;
#endif
      public:
        TraditionalDetourX86(cbfunc_t originalFunc, ptr_t newFunc);

      private:
        /**
         * @brief _callback_func
         * The call back function permits to call the original function
         * inside the new function.
         */
        cbfunc_t _callback_func {};
        /**
         * @brief _new_func
         * Pointer to the new function.
         */
        func_t _new_func {};
        func_t _original_func {};
        HandleFragment_t _handle_fragment {};
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
    template <calling_conventions_t calling_convention_T,
              typename ret_type_T,
              typename... args_T>
    constexpr auto TraditionalDetourX86<
      calling_convention_T,
      ret_type_T,
      args_T...>::GenerateCallBackFuncType()
    {
        if constexpr (calling_convention_T == cc_fastcall)
        {
            /* thisptr - EDX, stack params */
            return type_wrapper<ret_type_T(__thiscall*)(ptr_t, args_T...)>;
        }
        else if constexpr (calling_convention_T == cc_stdcall)
        {
            return type_wrapper<ret_type_T(__stdcall*)(args_T...)>;
        }
        else
        {
            return type_wrapper<ret_type_T (*)(args_T...)>;
        }
    }

    template <calling_conventions_t calling_convention_T,
              typename ret_type_T,
              typename... args_T>
    constexpr auto TraditionalDetourX86<calling_convention_T,
                                        ret_type_T,
                                        args_T...>::GenerateNewFuncType()
    {
        if constexpr (calling_convention_T == cc_fastcall)
        {
            /* EDX, ECX, stack params */
            return type_wrapper<ret_type_T(
              __fastcall*)(ptr_t, ptr_t, args_T...)>;
        }
        else if constexpr (calling_convention_T == cc_stdcall)
        {
            return type_wrapper<ret_type_T(__stdcall*)(args_T...)>;
        }
        else
        {
            return type_wrapper<ret_type_T (*)(args_T...)>;
        }
    }
#endif
} // namespace XKLib

#endif // DETOUR_H
