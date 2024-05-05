#include <stdio.h>
#include <stdbool.h>
#include <sys/scheduler.h>

int main(int argc, char** argv)
{
    if (argc != 2)
	{
        printf("Incorrect number of parameters.\n");
		return 1;
	}

    int pid = atoi(argv[1]);
    int slots = getProcessSlots(pid);

    if (slots == -1){
        printf("Error getting value from given process.\n");
        return 1;
    }
    else{
        printf("Process %d has %d slots.\n", pid, slots);
        return 0;
    }

}