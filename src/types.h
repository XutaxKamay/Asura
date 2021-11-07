#ifndef TYPES_H
#define TYPES_H

#define _MAKE_STRING(x) #x
#define MAKE_STRING(x)  _MAKE_STRING(x)

#define CURRENT_CONTEXT                                                  \
    "[XKLib][" __FILE__ ":" MAKE_STRING(__LINE__) "] -> "

namespace XKLib
{
    using ptr_t   = void*;
    using byte_t  = unsigned char;
    using data_t  = byte_t*;
    using bytes_t = std::vector<byte_t>;
#ifndef WINDOWS
    /* linux pid_t */
    using pid_t = int32_t;
#else
    /* windows uses dword urgh */
    using pid_t = int64_t;
#endif
#define __pid_t_defined

    template <typename T>
    struct type_wrapper_t
    {
        using type = T;
    };

    template <typename T>
    constexpr inline type_wrapper_t<T> type_wrapper {};

    template <typename T1, typename T2>
    auto view_as(T2 var)
    {
        return (T1)(var);
    }

} // namespace XKLib

#endif // TYPES_H
