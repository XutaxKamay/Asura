#ifndef MEMORYEXCEPTION_H
#define MEMORYEXCEPTION_H

#include <exception>
#include <iostream>

namespace XLib
{
    class MemoryException : std::exception
    {
      public:
        MemoryException(const std::string& msg)
        {
            _msg = msg;
        }

        auto& msg()
        {
            return _msg;
        }

      private:
        std::string _msg;
    };
};

#endif
