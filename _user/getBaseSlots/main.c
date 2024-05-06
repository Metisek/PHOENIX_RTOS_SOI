#include <stdio.h>
#include <stdbool.h>
#include <sys/scheduler.h>

int main(int argc, char** argv)
{
    int slots = getBaseSlots();
    printf("Base slots priority value: %d\n", slots);
    return 0;
}
