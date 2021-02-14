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

        struct Occurrence
        {
            byte_t value = 0;
            byte_t count = 0;
        };

        struct PathInfo
        {
            bool found      = false;
            size_t bit_path = 0;
            size_t depth    = 0;
        };

        struct Letter
        {
            byte_t value = 0;
            size_t freq  = 0;
        };

        struct Node
        {
            enum value_t
            {
                INVALID = -1
            };

            Node();

            auto height() -> size_t;
            auto countNodes() -> size_t;

            std::shared_ptr<Node> left;
            std::shared_ptr<Node> right;
            int value = INVALID;
            int id;
        };

        /**
         * @brief Tree
         * This tree tries to be a perfect binary tree.
         */
        struct Tree
        {
            std::shared_ptr<Node> root = std::shared_ptr<Node>(new Node());

            auto insert(Node* parent, byte_t value) -> void;
            auto insert(byte_t value) -> void;
            auto findPathInfo(PathInfo& pathInfo,
                              Node* parent,
                              byte_t value) -> void;
            auto findPathInfo(byte_t value) -> PathInfo;
            auto graphivzFormat(Node* parent) -> std::string;
            auto graphivzFormat() -> std::string;
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
        std::vector<Letter> _alphabet;
    };

};

#endif
