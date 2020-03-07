#ifndef DETOUR_H
#define DETOUR_H

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
#ifdef WINDOWS
        constexpr auto GenCallBackFunc()
        {
            if constexpr ( TCC == cc_fastcall )
            {
    #ifdef _WIN64
                /* RDX, RCX, stack params */
                return TRetType( __fastcall* )( ptr_t, ptr_t, TArgs... );
    #else
                /* RDX, stack params */
                return TRetType( __thiscall* )( ptr_t, TArgs... );
    #endif
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
#else
        using func_t = TRetType ( * )( TArgs... );
#endif

      public:
      private:
    };
}

#endif // DETOUR_H
