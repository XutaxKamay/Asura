#ifndef PATTERN_H
#define PATTERN_H

#include "process.h"

namespace XLib
{
    class Pattern
    {
      public:
        class Byte
        {
          public:
            Byte(int value);

            enum type_t
            {
                INVALID  = -1,
                UNKNOWN = -2
            };

            int value = INVALID;
        };

        Pattern(std::vector<Byte> bytes, std::vector<ptr_t> matches = {});

      public:
        auto bytes() -> std::vector<Byte>&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;

      private:
        std::vector<Byte> _bytes;
        std::vector<ptr_t> _matches;
    };

    using patterns_t = std::vector<Pattern>;
};

#endif
