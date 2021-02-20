#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <iostream>

#include "types.h"

namespace XLib
{
    class Exception : std::exception
    {
      public:
        Exception(const std::string& msg);

        auto msg() -> const std::string&;

      private:
        std::string _msg {};
    };

#define XLIB_EXCEPTION(msg)                                              \
    XLib::Exception(std::string(CURRENT_CONTEXT) + msg)
};

#endif
