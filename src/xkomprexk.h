#ifndef KONPRESS_H
#define KONPRESS_H

#include <algorithm>
#include <cmath>
#include <exception>
#include <list>
#include <memory>

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

        struct occurence_t
        {
            byte_t count = 0;
            byte_t value = 0;
        };

        struct path_info_t
        {
            bool found         = false;
            size_t bit_path    = 0;
            size_t height      = 0;
        };

        struct Node
        {
            enum value_t
            {
                INVALID = -1
            };

            auto height() -> size_t;

            std::shared_ptr<Node> left;
            std::shared_ptr<Node> right;
            int value = INVALID;
        };

        struct Tree
        {
            std::shared_ptr<Node> root = std::shared_ptr<Node>(new Node());

            auto insert(Node* parent, byte_t value) -> void;
            auto insert(byte_t value) -> void;
            auto find_path_info(path_info_t& path_info,
                                Node* parent,
                                byte_t value) -> void;
            auto find_path_info(byte_t value) -> path_info_t;
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
