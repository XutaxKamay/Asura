#ifndef OSUTILSEXCEPTION_H
#define OSUTILSEXCEPTION_H

#include <iostream>

namespace XLib
{
    class OSUtilsException : std::exception
    {
      public:
        OSUtilsException(const std::string& msg);

        auto msg() -> const std::string&;

      private:
        std::string _msg {};
    };
};

#endif
