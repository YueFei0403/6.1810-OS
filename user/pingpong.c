#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p2c[2]; 
    int c2p[2];
    char buf[1];

    // Create the pipes
    if (pipe(p2c) < 0 || pipe(c2p) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    } else if (pid == 0) { // Child process
        close(p2c[1]); // this is child process -> no parent write end
        close(c2p[0]);  // this is child process -> no parent read end

        // Read from parent
        if (read(p2c[0], buf, 1) != 1) {
            fprintf(2, "child read error\n");
            exit(1);
        }
        printf("%d: received ping\n", getpid());

        // Modify the buffer before sending back
        buf[0] = 'C';
        // Write back to parent
        if (write(c2p[1], buf, 1) != 1) {
            fprintf(2, "child write error\n");
            exit(1);
        }

        close(p2c[0]);
        close(c2p[1]);
        exit(0);
    } else { // Parent process
        close(p2c[0]); // this is parent process -> no child write end
        close(c2p[1]);  // this is parent process -> no child read end 

        // Send a byte to child
        buf[0] = 'P';
        if (write(p2c[1], buf, 1) != 1) {
            fprintf(2, "parent write error\n");
            exit(1);
        }

        // Wait for response from child
        if (read(c2p[0], buf, 1) != 1) {
            fprintf(2, "parent read error\n");
            exit(1);
        }

        printf("%d: received pong\n", getpid());
        close(p2c[1]);
        close(c2p[0]);
        wait(0); // Wait for child to finish
        exit(0);
    }

}
