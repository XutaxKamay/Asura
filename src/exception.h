#ifndef ASURA_EXCEPTION_H
#define ASURA_EXCEPTION_H

#include "types.h"

namespace Asura
{
    class Exception : std::exception
    {
      public:
        explicit Exception(std::string msg);

        auto msg() const -> const std::string&;

      private:
        std::string _msg {};
    };

#define ASURA_EXCEPTION(msg)                                             \
    throw Asura::Exception(std::string(CURRENT_CONTEXT) + (msg))
};

#endif
