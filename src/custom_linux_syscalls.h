#ifndef XKLIB_CUSTOM_LINUX_SYSCALLS_H
#define XKLIB_CUSTOM_LINUX_SYSCALLS_H

namespace XKLib
{
#ifdef __linux__
    constexpr auto __NR_custom_start   = 449;
    constexpr auto __NR_rmmap          = __NR_custom_start;
    constexpr auto __NR_rmprotect      = __NR_custom_start + 1;
    constexpr auto __NR_pkey_rmprotect = __NR_custom_start + 2;
    constexpr auto __NR_rmunmap        = __NR_custom_start + 3;
    constexpr auto __NR_rclone         = __NR_custom_start + 4;

    #if defined(__x86_64__) or defined(__i386__)
    template <typename T>
    concept ValidTypeForRegister = sizeof(T) <= sizeof(std::uintptr_t);

    template <ValidTypeForRegister... T, std::size_t N = sizeof...(T)>
    inline auto syscall_extended(const auto number, T... args)
    {
        std::uintptr_t ret;
        const auto targs = std::forward_as_tuple(args...);

        #if defined(__x86_64__)
        if constexpr (N == 0)
        {
            asm("syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)));
        }
        else if constexpr (N == 1)
        {
            asm("mov %2, %%rdi\n\t"
                "syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs)))
                : "rdi");
        }
        else if constexpr (N == 2)
        {
            asm("mov %2, %%rdi\n\t"
                "mov %3, %%rsi\n\t"
                "syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs)))
                : "rdi", "rsi");
        }
        else if constexpr (N == 3)
        {
            asm("mov %2, %%rdi\n\t"
                "mov %3, %%rsi\n\t"
                "mov %4, %%rdx\n\t"
                "syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs)))
                : "rdi", "rsi", "rdx");
        }
        else if constexpr (N == 4)
        {
            asm("mov %2, %%rdi\n\t"
                "mov %3, %%rsi\n\t"
                "mov %4, %%rdx\n\t"
                "mov %5, %%rcx\n\t"
                "syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<3>(targs)))
                : "rdi", "rsi", "rdx", "rcx");
        }
        else if constexpr (N == 5)
        {
            asm("mov %2, %%rdi\n\t"
                "mov %3, %%rsi\n\t"
                "mov %4, %%rdx\n\t"
                "mov %5, %%rcx\n\t"
                "mov %6, %%r8\n\t"
                "syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<3>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<4>(targs)))
                : "rdi", "rsi", "rdx", "rcx", "r8");
        }
        else if constexpr (N == 6)
        {
            asm("mov %2, %%rdi\n\t"
                "mov %3, %%rsi\n\t"
                "mov %4, %%rdx\n\t"
                "mov %5, %%rcx\n\t"
                "mov %6, %%r8\n\t"
                "mov %7, %%r9\n\t"
                "syscall\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<3>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<4>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<5>(targs)))
                : "rdi", "rsi", "rdx", "rcx", "r8", "r9");
        }
        #elif defined(__i386__)
        if constexpr (N == 0)
        {
            asm("int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)));
        }
        else if constexpr (N == 1)
        {
            asm("mov %2, %%ebx\n\t"
                "int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs)))
                : "ebx");
        }
        else if constexpr (N == 2)
        {
            asm("mov %2, %%ebx\n\t"
                "mov %3, %%ecx\n\t"
                "int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs)))
                : "ebx", "ecx");
        }
        else if constexpr (N == 3)
        {
            asm("mov %2, %%ebx\n\t"
                "mov %3, %%ecx\n\t"
                "mov %4, %%edx\n\t"
                "int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs)))
                : "ebx", "ecx", "edx");
        }
        else if constexpr (N == 4)
        {
            asm("mov %2, %%ebx\n\t"
                "mov %3, %%ecx\n\t"
                "mov %4, %%edx\n\t"
                "mov %5, %%esi\n\t"
                "int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<3>(targs)))
                : "ebx", "ecx", "edx", "esi");
        }
        else if constexpr (N == 5)
        {
            asm("mov %2, %%ebx\n\t"
                "mov %3, %%ecx\n\t"
                "mov %4, %%edx\n\t"
                "mov %5, %%esi\n\t"
                "mov %6, %%edi\n\t"
                "int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<3>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<4>(targs)))
                : "ebx", "ecx", "edx", "esi", "edi");
        }
        else if constexpr (N == 6)
        {
            asm("mov %2, %%ebx\n\t"
                "mov %3, %%ecx\n\t"
                "mov %4, %%edx\n\t"
                "mov %5, %%esi\n\t"
                "mov %6, %%edi\n\t"
                "mov %7, %%ebp\n\t"
                "int $0x80\n\t"
                : "=a"(ret)
                : "0"(view_as<std::uintptr_t>(number)),
                  "g"(view_as<std::uintptr_t>(std::get<0>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<1>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<2>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<3>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<4>(targs))),
                  "g"(view_as<std::uintptr_t>(std::get<5>(targs)))
                : "ebx", "ecx", "edx", "esi", "edi", "ebp");
        }
        #endif
        else
        {
            static_assert(N >= 0 and N <= 7,
                          "The argument count for the system call is not "
                          "valid.");
        }

        return ret;
    }
    #endif
#else
    #error "Not supported architecture"
#endif
}

#endif
