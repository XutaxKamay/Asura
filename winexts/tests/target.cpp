#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

unsigned char values[0x3000];

typedef void* ptr_t;


int main()
{
    memset(values, 0x11, sizeof(values));

    printf("pid: %i address: %lX value: %lX\n",
           getpid(),
           (uintptr_t)values,
           *(uintptr_t*)((uintptr_t)values + sizeof(values)
                         - sizeof(ptr_t)));

    while (1)
    {
        asm volatile("pushq $1000");
        asm volatile("pushq $0");
        asm volatile("mov %rsp, %rdi");
        asm volatile("mov $35, %rax");
        asm volatile("xor %esi, %esi");
        asm volatile("syscall");
    }
}
