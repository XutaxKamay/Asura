#ifndef KONPRESS_H
#define KONPRESS_H

#include <list>
#include <exception>

#include "bits.h"
#include "types.h"

namespace XLib
{
    class Komprex
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

        Komprex(data_t data, size_t size);

        auto compress() -> bytes_t;
        auto decompress() -> bytes_t;

      private:
        data_t _data;
        size_t _size;
        size_t _max_count_traversed_tree;
        std::list<byte_t> _alphabet;
    };

};

#endif
