#ifndef KERNEL_CUSTOM_UTILS
#define KERNEL_CUSTOM_UTILS

#include "main.h"

unsigned char hex_digit_to_byte(char c);
int hex_char_to_byte(char c1, char c2);
void swap_endian(unsigned char *addr, size_t len);

#endif