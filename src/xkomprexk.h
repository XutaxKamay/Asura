#ifndef KONPRESS_H
#define KONPRESS_H

#include <exception>
#include <list>

#include "bits.h"
#include "types.h"

namespace XLib
{
    class XKomprexk
    {
      public:
        class Exception : std::exception
        {
          public:
            Exception(const std::string& msg);

            auto msg() -> const std::string&;

          private:
            std::string _msg {};
        };

        struct freq_t
        {
            byte_t count;
            byte_t byte;
        };

      public:
        XKomprexk(data_t data, size_t size);

      public:
        auto compress() -> bytes_t;
        auto decompress() -> bytes_t;

      private:
        data_t _data;
        size_t _size;
        size_t _max_count_traversed_tree;
        std::vector<byte_t> _alphabet;
    };

};

#endif
