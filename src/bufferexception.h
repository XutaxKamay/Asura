#ifndef BUFFEREXCEPTION_H
#define BUFFEREXCEPTION_H

#include <iostream>

namespace XLib
{
    class BufferException : std::exception
    {
      public:
        BufferException(const std::string& msg);

        auto msg() -> const std::string&;

      private:
        std::string _msg {};
    };
};

#endif
