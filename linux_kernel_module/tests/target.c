#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int target_int = 0;

int main()
{
	while(1)
	{
		printf("%i %p\n", target_int, &target_int);
		sleep(1);
	}
}
