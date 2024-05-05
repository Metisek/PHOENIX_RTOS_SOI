#include <stdio.h>
#include <stdbool.h>
#include <sys/scheduler.h>

int main(int argc, char** argv)
{
    if (argc >= 2)
	{
		return 1;
	}

    printf("Running program in a infinite loop...");

    while (true)
    {
        continue;
    }

}
