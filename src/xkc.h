#ifndef XKLIB_XKC_H
#define XKLIB_XKC_H

#include "bits.h"
#include "buffer.h"
#include "types.h"

/**
 * This was done for fun, huffman is better
 **/
namespace XKLib
{
    template <typename T = byte_t>
    concept XKCAlphabetType = std::is_same<T, byte_t>::value or std::is_same<
      T,
      std::uint16_t>::value or std::is_same<T, std::uint32_t>::value;

    template <XKCAlphabetType T>
    class XKC
    {
      public:
        /* 64 bits because of -1 */
        using value_t = std::int64_t;

        using bit_path_t = std::bitset<std::numeric_limits<T>::max() + 1>;

        struct Occurrence
        {
            T letter_value;
            /* let's limit the occurences to 256 times */
            byte_t count {};
        };

        struct Letter
        {
            T value;
            std::size_t freq {};
        };

        struct PathInfo
        {
            bit_path_t bit_path {};
            std::size_t depth {};
        };

        struct PathInfoResult : PathInfo
        {
            T letter_value;
        };

        struct BinaryTree
        {
            struct Node
            {
                enum Value : value_t
                {
                    INVALID = -1
                };

                auto height() const -> std::size_t;
                auto depth() const -> std::size_t;
                auto count_subnodes() const -> std::size_t;

                std::shared_ptr<Node> root   = nullptr;
                std::shared_ptr<Node> parent = nullptr;
                value_t value                = INVALID;
                std::shared_ptr<Node> left   = nullptr;
                std::shared_ptr<Node> right  = nullptr;
            };

            BinaryTree();

            auto path_info(PathInfo& pathInfo,
                           const std::shared_ptr<Node> parent,
                           const T value) const -> bool;
            auto path_info(PathInfo& pathInfo, const T value) const
              -> bool;

            auto find_value(PathInfoResult& pathInfo) const -> void;

            auto dot_format(const std::shared_ptr<Node> parent) const
              -> std::string;
            auto dot_format() const -> std::string;

            auto insert(const std::shared_ptr<Node> parent, const T value)
              -> void;
            auto insert(const T value) -> void;

            std::shared_ptr<Node> root;
        };

        using alphabet_t    = std::vector<Letter>;
        using occurrences_t = std::vector<Occurrence>;

      public:
        static auto encode(const data_t data, const std::size_t size)
          -> bytes_t;
        static auto encode(const bytes_t& bytes) -> bytes_t;

        static auto decode(const data_t data, const std::size_t size)
          -> bytes_t;
        static auto decode(const bytes_t& bytes) -> bytes_t;
    };
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::Node::height() const -> std::size_t
{
    std::size_t height_left = 0, height_right = 0;

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

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::Node::depth() const -> std::size_t
{
    std::size_t result = 0;
    auto node          = parent;

    while (node)
    {
        result++;
        node = node->parent;
    }

    return result;
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::Node::count_subnodes() const
  -> std::size_t
{
    std::size_t result = 0;

    if (left)
    {
        result += left->count_subnodes();
        result++;
    }

    if (right)
    {
        result += right->count_subnodes();
        result++;
    }

    return result;
}

template <XKLib::XKCAlphabetType T>
XKLib::XKC<T>::BinaryTree::BinaryTree() : root(std::make_shared<Node>())
{
    root->root = root;
}

/**
 * The tree assumes that the alphabet was sorted
 * so the insertion of the most frequebnt value is the highest
 * in the tree
 */
template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::insert(const std::shared_ptr<Node> parent,
                                       const T value) -> void
{
    if (not parent->left)
    {
        parent->left = std::make_shared<Node>(
          Node { parent->root, parent, value });
        return;
    }
    else if (not parent->right)
    {
        parent->right = std::make_shared<Node>(
          Node { parent->root, parent, value });
        return;
    }
    else if (parent->left->count_subnodes()
             <= parent->right->count_subnodes())
    {
        insert(parent->left, value);
    }
    else
    {
        insert(parent->right, value);
    }
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::insert(const T value) -> void
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

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::encode(const bytes_t& bytes) -> XKLib::bytes_t
{
    return encode(view_as<data_t>(bytes.data()), bytes.size());
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::path_info(
  PathInfo& pathInfo,
  const std::shared_ptr<Node> parent,
  const T value) const -> bool
{
    if (parent == nullptr)
    {
        return false;
    }

    const auto depth = parent->depth();

    if (parent->value == value)
    {
        pathInfo.depth = depth;
        return true;
    }

    /* We've entered in one layer of the tree */
    const auto found_left = path_info(pathInfo, parent->left, value);

    if (found_left)
    {
        return true;
    }

    /* reset depth */
    const auto found_right = path_info(pathInfo, parent->right, value);

    if (found_right)
    {
        pathInfo.bit_path[depth] = 1;
        return true;
    }

    return found_right;
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::path_info(PathInfo& pathInfo,
                                          const T value) const -> bool
{
    return path_info(pathInfo, root, value);
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::find_value(PathInfoResult& pathInfo) const
  -> void
{
    if (root->height() < pathInfo.depth)
    {
        XKLIB_EXCEPTION("Can't go deeper than the height of the "
                        "tree.");
    }

    std::shared_ptr<Node> current_node = root;

    for (std::size_t depth = 0; depth < pathInfo.depth; depth++)
    {
        if (pathInfo.bit_path[depth])
        {
            current_node = current_node->right;
        }
        else
        {
            current_node = current_node->left;
        }

        if (current_node == nullptr)
        {
            XKLIB_EXCEPTION("The node doesn't exist!");
        }
    }

    pathInfo.letter_value = view_as<T>(current_node->value);
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::dot_format(
  const std::shared_ptr<Node> parent) const -> std::string
{
    std::string result;

    const auto max_depth_bits = bits_needed(parent->root->height());

    if (parent->left)
    {
        result += "\n";
        result += "\"" + std::string(1, parent->value) + " - ";

        std::string depth;

        for (std::size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->depth() & (1u << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (std::size_t depth_bit = 0; depth_bit < parent->depth();
             depth_bit++)
        {
            result += "x";
        }

        result += "\" -- \"" + std::string(1, parent->left->value)
                  + " - ";

        depth = "";

        for (std::size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->left->depth() & (1u << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (std::size_t depth_bit = 0; depth_bit < parent->left->depth();
             depth_bit++)
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

        for (std::size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->depth() & (1u << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (std::size_t depth_bit = 0; depth_bit < parent->depth();
             depth_bit++)
        {
            result += "x";
        }

        result += "\" -- \"" + std::string(1, parent->right->value)
                  + " - ";

        depth = "";

        for (std::size_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (parent->right->depth() & (1u << depth_bit))
            {
                depth = "1" + depth;
            }
            else
            {
                depth = "0" + depth;
            }
        }

        result += depth;

        for (std::size_t depth_bit = 0;
             depth_bit < parent->right->depth();
             depth_bit++)
        {
            result += "x";
        }

        result += std::string("\"") + " [label=1]";

        result += dot_format(parent->right);
    }

    return result;
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::BinaryTree::dot_format() const -> std::string
{
    std::string result = "strict graph {";

    result += dot_format(root);

    result += "\n}";

    return result;
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::encode(const data_t data, const std::size_t size)
  -> bytes_t
{
    bytes_t result;
    alphabet_t alphabet;
    occurrences_t occurrences;

    const auto values       = view_as<T*>(data);
    std::size_t value_index = 0;
    const auto max_values   = size / sizeof(T);

    /**
     * Store contiguous values
     */
    while (value_index < max_values)
    {
        std::size_t start_occurrence_index = value_index++;
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
    for (const auto& occurrence : occurrences)
    {
        const auto it = std::find_if(alphabet.begin(),
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
            alphabet.push_back({ occurrence.letter_value,
                                 occurrence.count });
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

    for (const auto& letter : alphabet)
    {
        binary_tree.insert(letter.value);
    }

    const auto max_tree_depth = binary_tree.root->height();

    const auto max_depth_bits = view_as<std::uint32_t>(
      bits_needed(max_tree_depth));

    const auto max_count_occurs = std::max_element(
      occurrences.begin(),
      occurrences.end(),
      [](Occurrence& a, Occurrence& b)
      {
          return (a.count < b.count);
      });

    const auto max_count_occurs_bits = view_as<byte_t>(
      bits_needed(max_count_occurs->count));

    result.push_back(max_count_occurs_bits);

    const auto tmp = view_as<std::uint32_t>(alphabet.size());
    const auto bytes_max_alphabet_size = view_as<byte_t*>(&tmp);

    for (std::size_t i = 0; i < sizeof(std::uint32_t); i++)
    {
        result.push_back(bytes_max_alphabet_size[i]);
    }

    for (const auto& letter : alphabet)
    {
        auto letter_value = view_as<T*>(&letter.value);

        for (std::size_t i = 0; i < sizeof(T); i++)
        {
            result.push_back(letter_value[i]);
        }
    }

    byte_t result_byte                      = 0;
    std::size_t written_bits_on_result_byte = 0;
    std::uint32_t written_bits              = 0;

    const auto check_bit = [&written_bits_on_result_byte,
                            &result,
                            &result_byte,
                            &written_bits]()
    {
        written_bits_on_result_byte++;
        written_bits++;

        if (written_bits_on_result_byte == CHAR_BIT)
        {
            result.push_back(result_byte);
            written_bits_on_result_byte = 0;
            result_byte                 = 0;
        }
    };

    const auto write_bit = [&result_byte, &written_bits_on_result_byte]()
    {
        result_byte |= (1u << written_bits_on_result_byte);
    };

    for (const auto& occurrence : occurrences)
    {
        PathInfo path_info;
        binary_tree.path_info(path_info, occurrence.letter_value);

        /**
         * TODO: find a better method to encode the depth
         */
        for (std::size_t count_bit = 0; count_bit < max_count_occurs_bits;
             count_bit++)
        {
            if (occurrence.count & (1u << count_bit))
            {
                write_bit();
            }

            check_bit();
        }

        for (std::uint32_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            if (path_info.depth & (1u << depth_bit))
            {
                write_bit();
            }

            check_bit();
        }

        for (std::size_t depth = 0; depth < path_info.depth; depth++)
        {
            if (path_info.bit_path[depth])
            {
                write_bit();
            }

            check_bit();
        }
    }

    if (written_bits_on_result_byte > 0)
    {
        result.push_back(result_byte);
    }

    const auto bytes_written_bits = view_as<byte_t*>(&written_bits);

    for (std::size_t i = 0; i < sizeof(std::uint32_t); i++)
    {
        result.push_back(bytes_written_bits[i]);
    }

    return result;
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::decode(const bytes_t& bytes) -> bytes_t
{
    return decode(view_as<data_t>(bytes.data()), bytes.size());
}

template <XKLib::XKCAlphabetType T>
auto XKLib::XKC<T>::decode(const data_t data, const std::size_t size)
  -> XKLib::bytes_t
{
    bytes_t result;

    std::size_t read_bytes = 0;

    auto written_bits = *view_as<std::uint32_t*>(
      view_as<std::uintptr_t>(data) + size - sizeof(std::uint32_t));

    if (written_bits / CHAR_BIT >= size)
    {
        XKLIB_EXCEPTION("there's too much bits to decode.");
    }

    const auto max_count_occurs_bits = data[read_bytes];
    read_bytes += sizeof(byte_t);

    const auto alphabet_size = *view_as<std::uint32_t*>(&data[read_bytes]);
    read_bytes += sizeof(std::uint32_t);

    alphabet_t alphabet;

    for (std::size_t alphabet_index = 0; alphabet_index < alphabet_size;
         alphabet_index++)
    {
        auto letter_value = *view_as<T*>(&data[read_bytes]);
        read_bytes += sizeof(T);

        alphabet.push_back({ letter_value });
    }

    BinaryTree binary_tree;

    /* Construct the tree */
    for (const auto& letter : alphabet)
    {
        binary_tree.insert(letter.value);
    }

    const auto max_tree_depth = binary_tree.root->height();

    const auto max_depth_bits = view_as<std::uint32_t>(
      bits_needed(max_tree_depth));

    std::size_t read_bits_on_result_byte = 0;
    std::size_t bits_read                = 0;
    occurrences_t occurrences;

    while (bits_read < written_bits)
    {
        const auto read_bit = [&read_bytes,
                               &read_bits_on_result_byte,
                               &bits_read,
                               &data,
                               &size]()
        {
            const auto value = (data[read_bytes]
                                & (1u << read_bits_on_result_byte)) ?
                                 1u :
                                 0;

            read_bits_on_result_byte++;
            bits_read++;

            if (read_bits_on_result_byte == CHAR_BIT)
            {
                read_bytes++;
                read_bits_on_result_byte = 0;

                if (read_bytes >= size)
                {
                    XKLIB_EXCEPTION("Too much bytes decoded.. "
                                    "Something is wrong.");
                }
            }

            return value;
        };

        PathInfoResult path_info;
        byte_t count = 0;

        for (std::size_t count_bit = 0; count_bit < max_count_occurs_bits;
             count_bit++)
        {
            count |= read_bit() << count_bit;
        }

        for (std::uint32_t depth_bit = 0; depth_bit < max_depth_bits;
             depth_bit++)
        {
            path_info.depth |= read_bit() << depth_bit;
        }

        for (std::size_t depth = 0; depth < path_info.depth; depth++)
        {
            path_info.bit_path[depth] = read_bit();
        }

        binary_tree.find_value(path_info);

        occurrences.push_back({ path_info.letter_value, count });
    }

    for (const auto& occurrence : occurrences)
    {
        for (std::size_t count = 0; count < occurrence.count; count++)
        {
            result.push_back(occurrence.letter_value);
        }
    }

    return result;
}

#endif
