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
        using bit_path_t = std::bitset<
          std::numeric_limits<alphabet_T>::max() + 1>;

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
            bit_path_t bit_path = 0;
            size_t depth        = 0;
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

            bool path_info(PathInfo& pathInfo,
                           shared_node parent,
                           alphabet_T value);

            bool path_info(PathInfo& pathInfo, alphabet_T value);

            std::string dot_format(shared_node parent);
            std::string dot_format();

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
        result += left->count_nodes();
        result++;
    }

    if (right)
    {
        result += right->count_nodes();
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
    if (!parent->left)
    {
        parent->left = shared_node(
          new Node({ parent->root, parent, value }));
        return;
    }
    else if (!parent->right)
    {
        parent->right = shared_node(
          new Node({ parent->root, parent, value }));
        return;
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
    if (root->value == Node::Value::INVALID)
    {
        root->value = value;
    }
    else
    {
        insert(root, value);
    }
}

template <typename alphabet_T>
XLib::bytes_t XLib::XKC<alphabet_T>::encode(XLib::bytes_t bytes)
{
    return encode(bytes.data(), bytes.size());
}

template <typename alphabet_T>
bool XLib::XKC<alphabet_T>::BinaryTree::path_info(PathInfo& pathInfo,
                                                  shared_node parent,
                                                  alphabet_T value)
{
    if (parent == nullptr)
    {
        return false;
    }

    if (parent->value == value)
    {
        pathInfo.depth = parent->depth();
        return true;
    }

    /* We've entered in one layer of the tree */
    auto found_left = path_info(pathInfo, parent->left, value);

    if (found_left)
    {
        pathInfo.bit_path[parent->depth()] = 0;
        return true;
    }

    pathInfo.bit_path[parent->depth()] = 1;

    /* reset depth */
    auto found_right = path_info(pathInfo, parent->right, value);

    if (found_right)
    {
        pathInfo.bit_path[parent->depth()] = 1;
        return true;
    }

    pathInfo.bit_path[parent->depth()] = 0;

    return found_right;
}

template <typename alphabet_T>
bool XLib::XKC<alphabet_T>::BinaryTree::path_info(PathInfo& pathInfo,
                                                  alphabet_T value)
{
    return path_info(pathInfo, root, value);
}

template <typename alphabet_T>
std::string XLib::XKC<alphabet_T>::BinaryTree::dot_format(
  shared_node parent)
{
    std::string result;

    auto max_depth_bits = BitsNeeded(parent->root->height() - 1);

    if (parent->left)
    {
        result += "\n";
        result += "\"" + std::string(1, parent->value) + " - ";

        std::string depth;

        for (size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->depth() & (1 << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (size_t depth = 0; depth < parent->depth(); depth++)
        {
            result += "x";
        }

        result += "\" -- \"" + std::string(1, parent->left->value)
                  + " - ";

        depth = "";

        for (size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->left->depth() & (1 << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (size_t depth = 0; depth < parent->left->depth(); depth++)
        {
            result += "x";
        }

        result += std::string("\"") + " [label=0]";
        result += dot_format(parent->left);
    }

    if (parent->right)
    {
        result += "\n";
        result += "\"" + std::string(1, parent->value) + " - ";

        std::string depth;

        for (size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->depth() & (1 << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (size_t depth = 0; depth < parent->depth(); depth++)
        {
            result += "x";
        }

        result += "\" -- \"" + std::string(1, parent->right->value)
                  + " - ";

        depth = "";

        for (size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->right->depth() & (1 << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (size_t depth = 0; depth < parent->right->depth(); depth++)
        {
            result += "x";
        }

        result += std::string("\"") + " [label=1]";

        result += dot_format(parent->right);
    }

    return result;
}

template <typename alphabet_T>
std::string XLib::XKC<alphabet_T>::BinaryTree::dot_format()
{
    std::string result = "strict graph {";

    result += dot_format(root);

    result += "\n}";

    return result;
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

    auto max_tree_depth = binary_tree.root->height() - 1;

    auto max_depth_bits = BitsNeeded(max_tree_depth);

    std::cout << max_tree_depth << " " << max_depth_bits << std::endl;

    std::cout << binary_tree.dot_format() << std::endl;

    byte_t result_byte                 = 0;
    size_t written_bits_on_result_byte = 0;
    size_t written_bits                = 0;

    std::string result_str;

    for (auto&& occurrence : occurrences)
    {
        for (size_t count = 0; count < occurrence.count; count++)
        {
            auto check_bit = [&written_bits_on_result_byte,
                              &result,
                              &result_byte,
                              &written_bits]()
            {
                written_bits_on_result_byte++;
                written_bits++;

                if (written_bits_on_result_byte == 8)
                {
                    result.push_back(result_byte);
                    written_bits_on_result_byte = 0;
                    result_byte                 = 0;
                }
            };

            auto write_bit =
              [&result_byte, &written_bits_on_result_byte]()
            {
                result_byte |= (1 << written_bits_on_result_byte);
            };

            PathInfo path_info;
            binary_tree.path_info(path_info, occurrence.letter_value);

            std::string depth;

            for (size_t depth_bit = 0; depth_bit < max_depth_bits;
                 depth_bit++)
            {
                if (path_info.depth & (1 << depth_bit))
                {
                    depth = "1" + depth;
                    write_bit();
                }
                else
                {
                    depth = "0" + depth;
                }

                check_bit();
            }

            result_str += depth;

            std::string bit_path;

            for (size_t depth = 0; depth < path_info.depth; depth++)
            {
                if (path_info.bit_path[depth])
                {
                    write_bit();
                    bit_path += "1";
                }
                else
                {
                    bit_path += "0";
                }

                check_bit();
            }

            result_str += bit_path + " ";
        }
    }

    std::cout << result_str << std::endl;

    std::cout << written_bits << std::endl;

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
