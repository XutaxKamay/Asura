#ifndef TEST_H
#define TEST_H

#include <xlib.h>
#include <stdarg.h>

namespace XLib
{
    class Test
    {
      public:
        class APIPure
        {
          public:
            virtual void func1()                                 = 0;
            virtual std::vector< int > func2( char const*, ... ) = 0;
        };

        class API : public APIPure
        {
            virtual void func1() override;
            virtual std::vector< int > func2( char const*, ... ) override;
        };

      public:
        static auto run() -> void;
        API m_API;

        static bool g_PassedTests;
    };
}

#endif
