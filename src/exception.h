#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <iostream>

#include "types.h"

namespace XKLib
{
    class Exception : std::exception
    {
      public:
        explicit Exception(std::string msg);

        auto msg() -> const std::string&;

      private:
        std::string _msg {};
    };

#define XLIB_EXCEPTION(msg)                                              \
    throw XKLib::Exception(std::string(CURRENT_CONTEXT) + (msg))
};

#endif
