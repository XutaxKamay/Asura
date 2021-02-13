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
    if (parent == nullptr)
    {
        return;
    }

    if (parent->left && !parent->right)
    {
        parent->right        = std::shared_ptr<Node>(new Node());
        parent->right->value = value;
    }
    else if (!parent->left && !parent->right)
    {
        parent->left        = std::shared_ptr<Node>(new Node());
        parent->left->value = value;
    }
    else
    {
        if (parent->left->height() >= parent->right->height())
        {
            insert(parent->left.get(), value);
        }
        else
        {
            insert(parent->right.get(), value);
        }
    }
}

auto XLib::XKomprexk::Tree::insert(XLib::byte_t value) -> void
{
    std::shared_ptr<Node> found_node;

    if (root->value == -1)
    {
        root->value = value;
    }
    else
    {
        if (root->left && !root->right)
        {
            root->right        = std::shared_ptr<Node>(new Node());
            root->right->value = value;
        }
        else if (!root->left && !root->right)
        {
            root->left        = std::shared_ptr<Node>(new Node());
            root->left->value = value;
        }
        else
        {
            if (root->left->height() >= root->right->height())
            {
                insert(root->left.get(), value);
            }
            else
            {
                insert(root->right.get(), value);
            }
        }
    }
}

auto XLib::XKomprexk::Tree::find_path_info(
  XLib::XKomprexk::path_info_t& path_info,
  XLib::XKomprexk::Node* parent,
  XLib::byte_t value) -> void
{
    if (parent == nullptr)
    {
        return;
    }

    if (parent->value == value)
    {
        path_info.found = true;
        return;
    }

    path_info.height += 1;

    auto old_path_info = path_info;
    find_path_info(path_info, parent->left.get(), value);

    if (path_info.found)
    {
        return;
    }

    path_info = old_path_info;

    path_info.bit_path |= (1 << path_info.height);
    find_path_info(path_info, parent->right.get(), value);
}

auto XLib::XKomprexk::Tree::find_path_info(XLib::byte_t value)
  -> XLib::XKomprexk::path_info_t
{
    path_info_t path_info;

    find_path_info(path_info, root.get(), value);

    return path_info;
}

XLib::XKomprexk::XKomprexk(XLib::data_t data, size_t size)
 : _data(data), _size(size)
{
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

    auto max_occurence_bits = *view_as<uint16_t*>(
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

    std::vector<occurence_t> occurences;

    while (read_size < _size)
    {
        occurence_t occurence;
        occurence.value = 0;
        occurence.count = 0;

        auto checkByte = [&read_size, &read_bits]()
        {
            read_bits++;

            if (read_bits == 8)
            {
                read_bits = 0;
                read_size++;
            }
        };

        auto bits_to_read = view_as<size_t>(max_occurence_bits
                                            + max_bits);
        auto bytes_left   = _size - read_size;

        if (bits_to_read > (8 - read_bits) && bytes_left <= 1)
        {
            break;
        }

        for (byte_t i = 0; i < max_occurence_bits; i++)
        {
            if (_data[read_size] & (1 << read_bits))
            {
                occurence.count |= (1 << i);
            }

            checkByte();
        }

        for (byte_t i = 0; i < max_bits; i++)
        {
            if (_data[read_size] & (1 << read_bits))
            {
                occurence.value |= (1 << i);
            }

            checkByte();
        }

        occurence.value = _alphabet[occurence.value];

        occurences.push_back(occurence);
    }

    for (auto&& occurence : occurences)
    {
        for (size_t i = 0; i < occurence.count; i++)
        {
            result.push_back(occurence.value);
        }
    }

    return result;
}

auto XLib::XKomprexk::compress() -> XLib::bytes_t
{
    bytes_t result;

    std::vector<occurence_t> occurences;

_alphabet.clear();

size_t i = 0;

while (i < _size)
{
    _alphabet.push_back(_data[i]);

    size_t start = i;

    occurence_t occurence;
    occurence.value = _data[start];
    occurence.count = 1;

    for (i++; i < _size; i++)
    {
        if (occurence.count
            == std::numeric_limits<decltype(occurence_t::count)>::max())
        {
            break;
        }

        if (_data[start] != _data[i])
        {
            break;
        }

        occurence.count++;
    }

    occurences.push_back(occurence);
}

std::sort(_alphabet.begin(), _alphabet.end());
_alphabet.erase(std::unique(_alphabet.begin(), _alphabet.end()),
                _alphabet.end());

auto occurence_count = occurences[0].count;

for (size_t i = 1; i < occurences.size(); i++)
{
    if (occurence_count < occurences[i].count)
    {
        occurence_count = occurences[i].count;
    }
}

auto alphabet_max_size = view_as<uint16_t>(_alphabet.size());

auto max_bits = view_as<uint16_t>(std::log2(alphabet_max_size - 1)) + 1;
auto max_occurence_bits = view_as<uint16_t>(std::log2(occurence_count))
                          + 1;

for (size_t i = 0; i < sizeof(uint16_t) * 3; i++)
{
    result.push_back(0);
}

size_t write_size                                                   = 0;
*view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) =
max_bits; write_size += sizeof(uint16_t);

*view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) =
max_occurence_bits; write_size += sizeof(uint16_t);

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

for (auto&& occurence : occurences)
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

    for (byte_t i = 0; i < max_occurence_bits; i++)
    {
        if (occurence.count & (1 << i))
        {
            result_byte |= (1 << written_bits_on_result_byte);
        }

        checkByte();
    }

    auto pos_alphabet = pos[occurence.value];

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
