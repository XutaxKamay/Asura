#ifndef BUFFEREXCEPTION_H
#define BUFFEREXCEPTION_H

#include "buffer.h"

namespace XLib
{
    class BufferException : std::exception
    {
      public:
        BufferException(const std::string& msg);

        auto& msg();

      private:
        std::string _msg {};
    };
};

#endif
