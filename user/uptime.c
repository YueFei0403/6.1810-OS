#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    int ticks = uptime();
    int secs = ticks / 10;
    int tenths = (ticks % 10);

    printf("Uptime: %d ticks (~%d.%d seconds)\n", ticks, secs, tenths);
    exit(0);
}