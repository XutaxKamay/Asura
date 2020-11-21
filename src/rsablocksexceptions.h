#ifndef RSABLOCKSEXCEPTIONS_H
#define RSABLOCKSEXCEPTIONS_H

#include "types.h"

namespace XLib
{
    class RSABlocksException : std::exception
    {
      public:
        RSABlocksException(const std::string& msg);

        auto msg() -> std::string&;

      private:
        std::string _msg {};
    };
};

#endif
