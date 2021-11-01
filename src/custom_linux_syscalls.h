#ifndef CUSTOM_LINUX_SYSCALLS
#define CUSTOM_LINUX_SYSCALLS

namespace XKLib
{
#ifndef _WIN32
    constexpr auto __NR_custom_start   = 449;
    constexpr auto __NR_rmmap          = __NR_custom_start;
    constexpr auto __NR_rmprotect      = __NR_custom_start + 1;
    constexpr auto __NR_pkey_rmprotect = __NR_custom_start + 2;
    constexpr auto __NR_rmunmap        = __NR_custom_start + 3;
    constexpr auto __NR_rclone         = __NR_custom_start + 4;
#endif
};

#endif
