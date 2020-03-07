#ifndef DETOUR_H
#define DETOUR_H

#include "types.h"

#if defined _WIN32 || defined _WIN64
    #define WINDOWS
#endif

#ifdef WINDOWS
enum calling_conventions_t
{
    cc_fastcall,
    cc_stdcall,
    cc_cdecl
};
#endif

namespace XLib
{
#ifdef WINDOWS
    template < calling_conventions_t TCC, typename TRetType, typename... TArgs >
#else
    template < typename TRetType, typename... TArgs >
#endif
    class Detour
    {
      private:
        /* This case is only for windows 32 bits program */
#ifdef _WIN32
        constexpr auto GenCallBackFunc()
        {
            if constexpr ( TCC == cc_fastcall )
            {
                /* thisptr - EDX, stack params */
                return TRetType( __thiscall* )( ptr_t, TArgs... );
            }
            else if constexpr ( TCC == cc_stdcall )
            {
                return TRetType( __stdcall* )( TArgs... );
            }
            else
            {
                return TRetType( * )( TArgs... );
            }
        }

      public:
        using func_t = typename decltype( GenCallBackFunc() );
#else /* Otherwhise it should be always the same convention */
        using func_t = TRetType ( * )( TArgs... );
#endif

      public:
      private:
    };
}

#endif // DETOUR_H
