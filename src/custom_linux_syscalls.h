#ifndef CUSTOM_LINUX_SYSCALLS
#define CUSTOM_LINUX_SYSCALLS

namespace XLib
{
#ifndef _WIN32
    constexpr auto __NR_rmmap          = 441;
    constexpr auto __NR_rmprotect      = 442;
    constexpr auto __NR_pkey_rmprotect = 443;
    constexpr auto __NR_rmunmap        = 444;
    constexpr auto __NR_rclone         = 445;
#endif
};

#endif
