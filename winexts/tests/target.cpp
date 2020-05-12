#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned char values[0x3000];

typedef void* ptr_t;

int main()
{
    memset(values, 0x11, sizeof(values));

    while (1)
    {
        printf("pid: %i address: %lX value: %lX\n",
               getpid(),
               (uintptr_t)values,
               *(uintptr_t*)((uintptr_t)values + sizeof(values)
                             - sizeof(ptr_t)));

        if (values[0] == 0x33)
        {
            *(int*)0x13371000 = 1337;
        }

        sleep(1);
    }
}
