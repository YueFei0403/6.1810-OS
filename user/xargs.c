#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// parse a line into words separated by spaces
int 
parse_args(char *line, char *args[], int start)
{
    int argc = start;
    char *p = line;
    int inword = 0;

    while (*p) {
        if (*p == ' ' || *p == '\t') {
            *p = 0;
            inword = 0;
        } else if (*p == '\n') {
            *p = 0;
            break;
        } else {
            if (!inword) {
                if (argc >= MAXARG-1) {
                    fprintf(2, "xargs: too many arguments\n");
                    exit(1);
                }
                args[argc++] = p;
                inword = 1;
            }
        }
        p++;
    }
    args[argc] = 0;
    return argc;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "Usage: xargs command [args...]\n");
        exit(1);
    }

    char buf[512];
    char *xargv[MAXARG];
    int i;
    int base_argc = argc - 1;

    // Read input line by line
    int n = 0;
    while (read(0, &buf[n], 1) == 1) {
        if (buf[n] == '\n') {
            // We read in the newline for later indications
            buf[n+1] = 0;

            // Copy base args
            // argv[0] = xargs itself
            // argv[1] = actual cmd. e.g. echo/find/grep...
            for (i = 0; i < base_argc; i++)
                xargv[i] = argv[i+1];

            // Parse line into words appended after base args
            int new_argc = parse_args(buf, xargv, base_argc);

            // Run the cmd: one line -> one exec
            if (fork() == 0) {
                exec(xargv[0], xargv);
                fprintf(2, "xargs: exec %s failed\n", xargv[0]);
                exit(1);
            }
            wait(0);

            // Reset buffer
            n = 0;
        } else {
            n++;
            if (n >= sizeof(buf)-2) {
                fprintf(2, "xargs: input line too long\n");
                exit(1);
            }
        }
    }
    exit(0);
}