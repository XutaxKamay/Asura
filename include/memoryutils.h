#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "memoryarea.h"
#include "memoryexception.h"
#include "types.h"

#ifndef WINDOWS
    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

namespace XLib
{
    /**
     * @brief MemoryUtils
     * Memory utils, mainly used for making compabilities between Linux
     * and Windows API.
     */
    class MemoryUtils
    {
      public:
        template <typename T>
        static constexpr inline auto align(T value, safesize_t size);

        template <typename T>
        static constexpr inline auto align_to_page_size(
          T sizeToAlign,
          safesize_t pageSize);

        template <typename T = uintptr_t>
        static auto ProtectMemoryArea(pid_t pid,
                                      T address,
                                      size_t size,
                                      memory_protection_flags_t flags)
          -> void;

        template <typename T = uintptr_t>
        static auto AllocArea(pid_t pid,
                              T address,
                              size_t size,
                              memory_protection_flags_t flags) -> ptr_t;

        template <typename T = uintptr_t>
        static auto FreeArea(pid_t pid, T address, size_t size = 0)
          -> void;

        template <typename T = uintptr_t>
        static auto ReadProcessMemoryArea(pid_t pid,
                                          T address,
                                          size_t size) -> bytes_t;

        template <typename T = uintptr_t>
        static auto WriteProcessMemoryArea(pid_t pid,
                                           bytes_t bytes,
                                           T address) -> void;

        static auto GetPageSize() -> size_t;

      private:
        static size_t _page_size;
    };

} // namespace XLib

#endif // MEMORYUTILS_H
