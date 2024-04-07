#include <stdio.h>
#include <sys/getpidlength.h>

int main(int argc, char** argv)
{
    pid_t pidToIgnore = -1;
	if (argc == 2)
	{
		pidToIgnore = atoi(argv[1]);
	}
    else if (argc >= 3)
	{
		return 1;
	}

    int longestPathLength = 0;
    longestPathLength = getPathLength(pidToIgnore);
    if (longestPathLength >= -1)
        printf("Length of the tree of a process having the longest path of descendants: %d\n", longestPathLength);
    else
        printf("Error while calling syscalls_getPathLength\n");

    return 0;
}
