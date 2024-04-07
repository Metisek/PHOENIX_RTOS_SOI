#include <stdio.h>
#include <sys/getpidlength.h>
#include <unistd.h>
#include <stdlib.h>

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

    // Wywołanie syscalls_getLongestPathPid
    int longestPathPid = 0;
    longestPathPid = getLongestPathPid(pidToIgnore);
    if (longestPathPid >= -1)
        printf("PID of the process having the longest path of descendants: %d\n", longestPathPid);
    else
        printf("Error while calling syscalls_getLongestPathPid\n");

    return 0;
}
