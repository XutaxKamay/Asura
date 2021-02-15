#ifndef XKC_H
#define XKC_H

#include "bits.h"
#include "buffer.h"
#include "types.h"

#include <algorithm>
#include <limits>
#include <memory>

namespace XLib
{
    template <typename alphabet_T = byte_t>
    class XKC
    {
      public:
        struct Occurrence
        {
            alphabet_T letter_value;
            /* let's limit the occurences to 256 times */
            byte_t count = 0;
        };

        struct Letter
        {
            alphabet_T value;
            size_t freq = 0;
        };

        struct PathInfo
        {
            std::bitset<std::numeric_limits<alphabet_T>::max() + 1>
              bit_path   = 0;
            size_t depth = 0;
        };

        struct BinaryTree
        {
            struct Node;
            using shared_node = std::shared_ptr<Node>;

            struct Node
            {
                static auto constexpr decide_alphabet_type()
                {
                    if constexpr (sizeof(alphabet_T) == 1)
                    {
                        return type_wrapper<int16_t>;
                    }
                    else if constexpr (sizeof(alphabet_T) == 2)
                    {
                        return type_wrapper<int32_t>;
                    }
                    else if constexpr (sizeof(alphabet_T) == 4)
                    {
                        return type_wrapper<int64_t>;
                    }
                    else
                    {
                        static_assert("Not supported");
                    }
                }

                using value_t = typename decltype(
                  decide_alphabet_type())::type;

                enum Value : value_t
                {
                    INVALID = -1
                };

                auto height() -> size_t;
                auto depth() -> size_t;
                auto count_nodes() -> size_t;

                shared_node root   = nullptr;
                shared_node parent = nullptr;
                value_t value      = INVALID;
                shared_node left   = nullptr;
                shared_node right  = nullptr;
            };

            BinaryTree();

            void insert(shared_node parent, alphabet_T value);
            void insert(alphabet_T value);

            void path_info(shared_node parent, alphabet_T value);
            void path_info(alphabet_T value);

            /* Plant the seed when tree is created */
            shared_node root;
        };

        using alphabet_t    = std::vector<Letter>;
        using occurrences_t = std::vector<Occurrence>;

      public:
        static bytes_t encode(data_t data, size_t size);
        static bytes_t encode(bytes_t bytes);

        static bytes_t decode(data_t data, size_t size);
        static bytes_t decode(bytes_t bytes);
    };
};

template <typename alphabet_T>
size_t XLib::XKC<alphabet_T>::BinaryTree::Node::count_nodes()
{
    size_t result = 0;

    if (left)
    {
        result = left->count_nodes();
        result++;
    }

    if (right)
    {
        result = right->count_nodes();
        result++;
    }

    return result;
}

template <typename alphabet_T>
size_t XLib::XKC<alphabet_T>::BinaryTree::Node::depth()
{
    size_t result = 0;
    auto node     = parent;

    while (node)
    {
        result++;
        node = node->parent;
    }

    return result;
}

template <typename alphabet_T>
size_t XLib::XKC<alphabet_T>::BinaryTree::Node::height()
{
    size_t height_left = 0, height_right = 0;

    if (left)
    {
        height_left = left->height();
        height_left++;
    }

    if (right)
    {
        height_right = right->height();
        height_right++;
    }

    return std::max(height_left, height_right);
}

template <typename alphabet_T>
XLib::XKC<alphabet_T>::BinaryTree::BinaryTree() : root(new Node())
{
    root->root = root;
}

/**
 * The tree assumes that the alphabet was sorted
 * so the insertion of the most frequebnt value is the highest
 * in the tree
 */
template <typename alphabet_T>
void XLib::XKC<alphabet_T>::BinaryTree::insert(shared_node parent,
                                               alphabet_T value)
{
    if (parent->value == Node::Value::INVALID)
    {
        parent->value = value;
    }
    else if (!parent->left)
    {
        parent->left = shared_node(
          new Node({ parent->root, parent, value }));
    }
    else if (!parent->right)
    {
        parent->right = shared_node(
          new Node({ parent->root, parent, value }));
    }
    else if (parent->left->count_nodes() <= parent->right->count_nodes())
    {
        insert(parent->left, value);
    }
    else
    {
        insert(parent->right, value);
    }
}

template <typename alphabet_T>
void XLib::XKC<alphabet_T>::BinaryTree::insert(alphabet_T value)
{
    insert(root, value);
}

template <typename alphabet_T>
XLib::bytes_t XLib::XKC<alphabet_T>::encode(XLib::bytes_t bytes)
{
    return encode(bytes.data(), bytes.size());
}

template <typename alphabet_T>
XLib::bytes_t XLib::XKC<alphabet_T>::encode(XLib::data_t data,
                                            size_t size)
{
    bytes_t result;
    alphabet_t alphabet;
    occurrences_t occurrences;

    auto values        = view_as<alphabet_T*>(data);
    size_t value_index = 0;

    auto max_values = size / sizeof(alphabet_T);

    /**
     * Store contiguous values
     */
    while (value_index < max_values)
    {
        size_t start_occurrence_index = value_index++;

        Occurrence occurrence;
        occurrence.letter_value = values[start_occurrence_index];
        occurrence.count        = 1;

        for (; value_index < max_values; value_index++)
        {
            if (occurrence.count
                == std::numeric_limits<decltype(Occurrence::count)>::max())
            {
                break;
            }

            if (values[start_occurrence_index] != values[value_index])
            {
                break;
            }

            occurrence.count++;
        }

        occurrences.push_back(occurrence);
    }

    /* Construct the alphabet */
    for (auto&& occurrence : occurrences)
    {
        auto it = std::find_if(alphabet.begin(),
                               alphabet.end(),
                               [&occurrence](Letter& a)
                               {
                                   return (occurrence.letter_value
                                           == a.value);
                               });

        if (it != alphabet.end())
        {
            it->freq += occurrence.count;
        }
        else
        {
            alphabet.push_back(
              { occurrence.letter_value, occurrence.count });
        }
    }

    /* Sort by highest frequency */
    std::sort(alphabet.begin(),
              alphabet.end(),
              [](Letter& a, Letter& b)
              {
                  return (a.freq > b.freq);
              });

    BinaryTree binary_tree;

    for (auto&& letter : alphabet)
    {
        binary_tree.insert(letter.value);
    }

    auto tree_max_depth = binary_tree.root->depth();

    auto max_depth_bits = BitsNeeded(tree_max_depth);

    return result;
}

template <typename alphabet_T>
XLib::bytes_t XLib::XKC<alphabet_T>::decode(XLib::bytes_t bytes)
{
    return decode(bytes.data(), bytes.size());
}

template <typename alphabet_T>
XLib::bytes_t XLib::XKC<alphabet_T>::decode(XLib::data_t data,
                                            size_t size)
{
    bytes_t result;
    return result;
}

#endif
