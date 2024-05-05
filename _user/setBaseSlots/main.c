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

    int slots = atoi(argv[1]);

    if (setBaseSlots(slots) == 0){
        printf("Error setting value of base slots.\n");
        return 1;
    }
    else{
        printf("Base slots set to: %d\n", slots);
        return 0;
    }

}
