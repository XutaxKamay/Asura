#ifndef MEMORYEXCEPTION_H
#define MEMORYEXCEPTION_H

#include <exception>
#include <iostream>

namespace XLib
{
    class MemoryException : std::exception
    {
      public:
        MemoryException(const std::string& msg);

        auto msg() -> std::string&;

      private:
        std::string _msg {};
    };
};

#endif
