#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int target_int = 0;

int main()
{
	while(1)
	{
		printf("pid: %i value: %i address: %p\n", getpid(), target_int, &target_int);
		sleep(1);
	}
}
