#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <iostream>
#include <exception>

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
};

#endif
