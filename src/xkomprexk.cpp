#include "xkomprexk.h"
#include "networkreadbuffer.h"
#include "networkwritebuffer.h"

XLib::XKomprexk::Exception::Exception(const std::string& msg) : _msg(msg)
{
}

const std::string& XLib::XKomprexk::Exception::msg()
{
    return _msg;
}

XLib::XKomprexk::Node::Node()
{
    static int count_id = 0;

    id = count_id++;
}

size_t XLib::XKomprexk::Node::countNodes()
{
    size_t result = 0;

    if (left)
    {
        result += left->countNodes() + 1;
    }

    if (right)
    {
        result += right->countNodes() + 1;
    }

    return result;
}

auto XLib::XKomprexk::Node::height() -> size_t
{
    size_t height_left = 0, height_right = 0;

    if (left)
    {
        height_left = left->height() + 1;
    }

    if (right)
    {
        height_right = right->height() + 1;
    }

    return std::max(height_left, height_right);
}

auto XLib::XKomprexk::Tree::insert(XLib::XKomprexk::Node* parent,
                                   XLib::byte_t value) -> void
{
    if (!parent->left)
    {
        parent->left        = std::shared_ptr<Node>(new Node());
        parent->left->value = value;
        return;
    }
    else if (!parent->right)
    {
        parent->right        = std::shared_ptr<Node>(new Node());
        parent->right->value = value;
        return;
    }
    else
    {
        if (parent->left->countNodes() > parent->right->countNodes())
        {
            insert(parent->right.get(), value);
        }
        else
        {
            insert(parent->left.get(), value);
        }
    }
}

auto XLib::XKomprexk::Tree::insert(XLib::byte_t value) -> void
{
    std::shared_ptr<Node> found_node;

    if (root->value == Node::value_t::INVALID)
    {
        root->value = value;
    }
    else
    {
        insert(root.get(), value);
    }
}

auto XLib::XKomprexk::Tree::findPathInfo(
  XLib::XKomprexk::PathInfo& pathInfo,
  XLib::XKomprexk::Node* parent,
  XLib::byte_t value) -> void
{
    if (parent == nullptr)
    {
        return;
    }

    if (parent->value == value)
    {
        pathInfo.found = true;
        return;
    }

    pathInfo.depth += 1;

    auto old_path_info = pathInfo;
    findPathInfo(pathInfo, parent->left.get(), value);

    if (pathInfo.found)
    {
        return;
    }

    pathInfo = old_path_info;

    pathInfo.bit_path |= (1 << pathInfo.depth);
    findPathInfo(pathInfo, parent->right.get(), value);
}

std::string XLib::XKomprexk::Tree::graphivzFormat(
  XLib::XKomprexk::Node* parent)
{
    std::string result;

    if (parent->left)
    {
        result += "\n";
        result += std::to_string(parent->id) + " -- "
                  + std::to_string(parent->left->id);
        result += graphivzFormat(parent->left.get());
    }

    if (parent->right)
    {
        result += "\n";
        result += std::to_string(parent->id) + " -- "
                  + std::to_string(parent->right->id);

        result += graphivzFormat(parent->right.get());
    }

    return result;
}

std::string XLib::XKomprexk::Tree::graphivzFormat()
{
    std::string result = "strict graph {";

    auto left = root->left;

    if (left)
    {
        result += "\n";
        result += std::to_string(root->id) + " -- "
                  + std::to_string(left->id);
        result += graphivzFormat(left.get());
    }

    auto right = root->right;

    if (right)
    {
        result += "\n";
        result += std::to_string(root->id) + " -- "
                  + std::to_string(right->id);

        result += graphivzFormat(right.get());
    }

    result += "\n}";

    return result;
}

auto XLib::XKomprexk::Tree::findPathInfo(XLib::byte_t value)
  -> XLib::XKomprexk::PathInfo
{
    PathInfo path_info;

    findPathInfo(path_info, root.get(), value);

    return path_info;
}

XLib::XKomprexk::XKomprexk(XLib::data_t data, size_t size)
 : _data(data), _size(size)
{
}

auto XLib::XKomprexk::compress() -> XLib::bytes_t
{
    bytes_t result;

    std::vector<Occurrence> occurrences;

    _alphabet.clear();

    size_t i = 0;

    while (i < _size)
    {
        size_t start = i++;

        Occurrence occurrence;
        occurrence.value = _data[start];
        occurrence.count = 1;

        for (; i < _size; i++)
        {
            if (occurrence.count
                == std::numeric_limits<decltype(Occurrence::count)>::max())
            {
                break;
            }

            if (_data[start] != _data[i])
            {
                break;
            }

            occurrence.count++;
        }

        occurrences.push_back(occurrence);
    }

    std::sort(occurrences.begin(),
              occurrences.end(),
              [](Occurrence& a, Occurrence& b)
              {
                  return (a.count > b.count);
              });

    for (auto&& occurrence : occurrences)
    {
        auto it = std::find_if(_alphabet.begin(),
                               _alphabet.end(),
                               [&occurrence](Letter& a)
                               {
                                   return (occurrence.value == a.value);
                               });

        if (it != _alphabet.end())
        {
            it->freq += occurrence.count;
        }
        else
        {
            _alphabet.push_back({ occurrence.value, occurrence.count });
        }
    }

    std::sort(_alphabet.begin(),
              _alphabet.end(),
              [](Letter& a, Letter& b)
              {
                  return (a.freq > b.freq);
              });

    Tree tree;

    for (auto&& letter : _alphabet)
    {
        tree.insert(letter.value);
    }

    auto max_alphabet_bits = BitsNeeded(_alphabet.size());
    auto max_depth_bits    = BitsNeeded(tree.root->height());

    std::cout << max_depth_bits << " " << max_alphabet_bits << std::endl;

    std::cout << tree.graphivzFormat() << std::endl;

    return result;
}

auto XLib::XKomprexk::decompress() -> XLib::bytes_t
{
    bytes_t result;
    return result;
}

/*
auto XLib::XKomprexk::decompress() -> XLib::bytes_t
{
    bytes_t result;

    _alphabet.clear();

    size_t read_size = 0;

    auto max_bits = *view_as<uint16_t*>(view_as<uintptr_t>(_data)
                                        + read_size);
    read_size += sizeof(uint16_t);

    auto max_occurrence_bits = *view_as<uint16_t*>(
      view_as<uintptr_t>(_data) + read_size);
    read_size += sizeof(uint16_t);

    auto alphabet_max_size = *view_as<uint16_t*>(view_as<uintptr_t>(_data)
                                                 + read_size);
    read_size += sizeof(uint16_t);

    for (uint16_t i = 0; i < alphabet_max_size; i++)
    {
        _alphabet.push_back(_data[read_size]);
        read_size++;
    }

    size_t read_bits = 0;

    std::vector<Occurrence> occurrences;

    while (read_size < _size)
    {
        Occurrence occurrence;
        occurrence.value = 0;
        occurrence.count = 0;

        auto checkByte = [&read_size, &read_bits]()
        {
            read_bits++;

            if (read_bits == 8)
            {
                read_bits = 0;
                read_size++;
            }
        };

        auto bits_to_read = view_as<size_t>(max_occurrence_bits
                                            + max_bits);
        auto bytes_left   = _size - read_size;

        if (bits_to_read > (8 - read_bits) && bytes_left <= 1)
        {
            break;
        }

        for (byte_t i = 0; i < max_occurrence_bits; i++)
        {
            if (_data[read_size] & (1 << read_bits))
            {
                occurrence.count |= (1 << i);
            }

            checkByte();
        }

        for (byte_t i = 0; i < max_bits; i++)
        {
            if (_data[read_size] & (1 << read_bits))
            {
                occurrence.value |= (1 << i);
            }

            checkByte();
        }

        occurrence.value = _alphabet[occurrence.value];

        occurrences.push_back(occurrence);
    }

    for (auto&& occurrence : occurrences)
    {
        for (size_t i = 0; i < occurrence.count; i++)
        {
            result.push_back(occurrence.value);
        }
    }

    return result;
}

auto XLib::XKomprexk::compress() -> XLib::bytes_t
{
    bytes_t result;

    std::vector<Occurrence> occurrences;

_alphabet.clear();

size_t i = 0;

while (i < _size)
{
    _alphabet.push_back(_data[i]);

    size_t start = i;

    Occurrence occurrence;
    occurrence.value = _data[start];
    occurrence.count = 1;

    for (i++; i < _size; i++)
    {
        if (occurrence.count
            == std::numeric_limits<decltype(Occurrence::count)>::max())
        {
            break;
        }

        if (_data[start] != _data[i])
        {
            break;
        }

        occurrence.count++;
    }

    occurrences.push_back(occurrence);
}

std::sort(_alphabet.begin(), _alphabet.end());
_alphabet.erase(std::unique(_alphabet.begin(), _alphabet.end()),
                _alphabet.end());

auto occurrence_count = occurrences[0].count;

for (size_t i = 1; i < occurrences.size(); i++)
{
    if (occurrence_count < occurrences[i].count)
    {
        occurrence_count = occurrences[i].count;
    }
}

auto alphabet_max_size = view_as<uint16_t>(_alphabet.size());

auto max_bits = view_as<uint16_t>(std::log2(alphabet_max_size - 1)) + 1;
auto max_occurrence_bits = view_as<uint16_t>(std::log2(occurrence_count))
                          + 1;

for (size_t i = 0; i < sizeof(uint16_t) * 3; i++)
{
    result.push_back(0);
}

size_t write_size                                                   = 0;
*view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) =
max_bits; write_size += sizeof(uint16_t);

*view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) =
max_occurrence_bits; write_size += sizeof(uint16_t);

*view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) =
alphabet_max_size; write_size += sizeof(uint16_t);

std::vector<byte_t> pos(std::pow<size_t>(2, 8 * sizeof(byte_t)));

size_t letter_pos = 0;

for (auto&& letter : _alphabet)
{
    result.push_back(letter);
    pos[letter] = letter_pos;

    letter_pos++;
}

byte_t result_byte                 = 0;
size_t written_bits_on_result_byte = 0;

for (auto&& occurrence : occurrences)
{
    auto checkByte =
      [&written_bits_on_result_byte, &result, &result_byte]()
    {
        written_bits_on_result_byte++;

        if (written_bits_on_result_byte == 8)
        {
            result.push_back(result_byte);
            written_bits_on_result_byte = 0;
            result_byte                 = 0;
        }
    };

    for (byte_t i = 0; i < max_occurrence_bits; i++)
    {
        if (occurrence.count & (1 << i))
        {
            result_byte |= (1 << written_bits_on_result_byte);
        }

        checkByte();
    }

    auto pos_alphabet = pos[occurrence.value];

    for (byte_t i = 0; i < max_bits; i++)
    {
        if (pos_alphabet & (1 << i))
        {
            result_byte |= (1 << written_bits_on_result_byte);
        }

        checkByte();
    }
}

if (written_bits_on_result_byte > 0)
{
    result.push_back(result_byte);
}

return result;
}
*/
