#include <stdio.h>
#include <sys/getpidlength.h>
#include <unistd.h>
#include <stdlib.h>

#define SYSCALL_GET_LONGEST_PATH_PID 437

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
    if (longestPathPid >= -999)
        printf("PID procesu mającego najdłuższą ścieżkę potomków: %d\n", longestPathPid);
    else
        printf("Błąd podczas wywoływania syscalls_getLongestPathPid\n");

    return 0;
}
