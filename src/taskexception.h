#ifndef TASKEXCEPTION_H
#define TASKEXCEPTION_H

#include <exception>
#include <iostream>

namespace XLib
{
    class TaskException : std::exception
    {
      public:
        TaskException(const std::string& msg);

        auto& msg();

      private:
        std::string _msg {};
    };
};

#endif
