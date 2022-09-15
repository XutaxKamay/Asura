#ifndef ASURA_BUILTINS_H
#define ASURA_BUILTINS_H

#include "types.h"

namespace Asura
{
    class Builtins
    {
      public:
        static inline auto CTZ(const auto var)
        {
            if constexpr (sizeof(var) <= sizeof(std::int32_t))
            {
                return __builtin_ctz(var);
            }
            else
            {
                return __builtin_ctzll(var);
            }
        }

        static inline auto FFS(const auto var)
        {
            if constexpr (sizeof(var) <= sizeof(std::int32_t))
            {
                return view_as<std::uint32_t>(__builtin_ffs(var));
            }
            else
            {
                return view_as<std::uint64_t>(__builtin_ffsll(var));
            }
        }
    };
}

#endif
