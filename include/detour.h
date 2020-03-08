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
#ifdef _WIN32
    template < calling_conventions_t TCC, typename TRetType, typename... TArgs >
#else
    template < typename TRetType, typename... TArgs >
#endif
    class Detour
    {
      private:
        /* This case is only for windows 32 bits program */
#ifdef _WIN32
        constexpr auto GenCallBackFuncType()
        {
            if constexpr ( TCC == cc_fastcall )
            {
                /* thisptr - EDX, stack params */
                return type_wrapper< TRetType( __thiscall* )( ptr_t, TArgs... ) >;
            }
            else if constexpr ( TCC == cc_stdcall )
            {
                return type_wrapper< TRetType( __stdcall* )( TArgs... ) >;
            }
            else
            {
                return type_wrapper< TRetType ( * )( TArgs... ) >;
            }
        }

        constexpr auto GenNewFuncType()
        {
            if constexpr ( TCC == cc_fastcall )
            {
                /* EDX, ECX, stack params */
                return type_wrapper< TRetType(
                  __fastcall* )( ptr_t, ptr_t, TArgs... ) >;
            }
            else if constexpr ( TCC == cc_stdcall )
            {
                return type_wrapper< TRetType( __stdcall* )( TArgs... ) >;
            }
            else
            {
                return type_wrapper< TRetType ( * )( TArgs... ) >;
            }
        }

      public:
        using cbfunc_t = typename decltype( GenCallBackFuncType() )::type;
        using func_t   = typename decltype( GenNewFuncType() )::type;

#else /* Otherwhise it should be always the same convention */
        using cbfunc_t = TRetType ( * )( TArgs... );
        using func_t   = cbfunc_t;
#endif

      public:
      private:
        cbfunc_t _callBackFunc;
        func_t _newFunc;
    };
}

#endif // DETOUR_H
