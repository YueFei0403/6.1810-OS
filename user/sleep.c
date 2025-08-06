#include "kernel/types.h"
#include "user/user.h"

#define HZ 10 // Confirmed  10Hz from clockintr()

// Sleep for 'seconds' (converts to ticks)
// Math: 10 ticks per second
int 
main(int argc, char *argv[])
{
    if (argc != 2){
        fprintf(2, "Usage: sleep seconds\n");
        exit(1);
    }
    int ticks = atoi(argv[1]) * HZ;
    sleep(ticks);
    exit(0);
}
