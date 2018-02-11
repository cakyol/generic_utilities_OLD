
#include <stdio.h>
#include <time.h>

int main (int argc, char *argv[])
{
    struct timespec res;

    if (clock_getres(CLOCK_REALTIME, &res) == 0)  {
	printf("system clock resolution secs: %d\n", res.tv_sec);
	printf("system clock resolution nano secs: %ld\n", res.tv_nsec);
    }
}


