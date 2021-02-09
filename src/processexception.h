#ifndef PROCESSEXCEPTION_H
#define PROCESSEXCEPTION_H

#include <exception>
#include <iostream>

namespace XLib
{
    class ProcessException : std::exception
    {
      public:
          ProcessException(const std::string& msg);

        auto& msg();

      private:
        std::string _msg {};
    };
};

#endif
