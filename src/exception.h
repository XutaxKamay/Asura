#ifndef XKLIB_EXCEPTION_H
#define XKLIB_EXCEPTION_H

#include "types.h"

namespace XKLib
{
    class Exception : std::exception
    {
      public:
        explicit Exception(std::string msg);

        auto msg() const -> const std::string&;

      private:
        std::string _msg {};
    };

#define XKLIB_EXCEPTION(msg)                                             \
    throw XKLib::Exception(std::string(CURRENT_CONTEXT) + (msg))
};

#endif
