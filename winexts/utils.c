#include "main.h"

unsigned char hex_digit_to_byte(char c)
{
    if (c >= '0' && c <= '9')
    {
        c -= '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
        c -= 'A';
        c += 10;
    }
    else if (c >= 'a' && c <= 'f')
    {
        c -= 'a';
        c += 10;
    }
    else
    {
        c = 0xFF;
    }

    return c;
}

int hex_char_to_byte(char c1, char c2)
{
    c1 = hex_digit_to_byte(c1);
    c2 = hex_digit_to_byte(c2);

    if ((unsigned char)c1 == 0xFF || (unsigned char)c2 == 0xFF)
        return 0x100; // Wrong char.

    return ((int)c2 + ((int)c1 * 0x10));
}

void swap_endian(unsigned char* addr, size_t len)
{
    int i;
    unsigned char backup_byte;

    for (i = 0; i < len / 2; i++)
    {
        backup_byte         = addr[i];
        addr[i]             = addr[(len - 1) - i];
        addr[(len - 1) - i] = backup_byte;
    }
}

int convert_to_hexstring(uint8_t* array,
                         size_t size,
                         char output[],
                         size_t output_size)
{
    int i, j;
    const char* format;

    format = "%02X ";

    for (i = 0, j = 0; i < size; i++)
    {
        if (j + 3 > output_size)
        {
            return 0;
        }

        sprintf(&output[j], format, array[i]);
        j += 3;
    }

    if (j > output_size)
    {
        return 0;
    }

    output[j] = '\0';

    return 1;
}
