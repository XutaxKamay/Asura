#ifndef XKLIB_BUILTINS_H
#define XKLIB_BUILTINS_H

namespace XKLib
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
                return __builtin_ffs(var);
            }
            else
            {
                return __builtin_ffsll(var);
            }
        }
    };
}

#endif
