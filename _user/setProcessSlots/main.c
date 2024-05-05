#include <stdio.h>
#include <stdbool.h>
#include <sys/scheduler.h>

int main(int argc, char** argv)
{
    if (argc != 3)
	{
        printf("Incorrect number of parameters.\n");
		return 1;
	}

    int pid = atoi(argv[1]);
    int slots = atoi(argv[2]);

    if (setProcessSlots(pid, slots) == 0){
        printf("Error setting value of slots for process %d.\n", pid);
        return 1;
    }
    else{
        printf("Base slots for process %d set to: %d\n", pid, slots);
        return 0;
    }

}