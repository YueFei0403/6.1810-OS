// #include "kernel/types.h"
// #include "user/user.h"

// // static int report_wfd = -1; // global collector write-end

// /************ File-lcoal functions *********/
// static int read_int(int fd, int *v) {
//     int n = read(fd, v, sizeof(*v));
//     return n == sizeof(*v) ? 1 : 0;
// }

// static void write_int(int fd, int v) {
//     write(fd, &v, sizeof(v));
// }

// static void filter_for_p(int leftfd, int rightfd, int p) {
//     int x;
//     while (read_int(leftfd, &x)) {
//         if (x % p != 0) {
//             write_int(rightfd, x);
//         }
//     }
//     close(leftfd);
//     close(rightfd);
// }

// /***********  Sieve Stage ***********/
// void primes(int leftfd) __attribute__((noreturn));
// void primes(int leftfd) {
//    int p;
//    if (!read_int(leftfd, &p)) {
//     close(leftfd);
//     exit(0);
//    }
   
//    // First value at this stage is prime
//    printf("prime %d\n", p);

//    int right[2];
//    pipe(right);
//    int pid = fork();
//    if (pid == 0) {
//     // Child: next stage
//     close(right[1]);
//     close(leftfd);
//     primes(right[0]); // no return
//    } else {
//     // Parent: filter multiples of p and feed child
//     close(right[0]);
//     filter_for_p(leftfd, right[1], p);
//     wait(0);
//     exit(0);
//    }
// }


// /************* Generator ************/
// int main(void) {
//     int in_data[2];
//     pipe(in_data);
//     int pid = fork();
//     if (pid == 0) {
//         // Child: run sieve
//         close(in_data[1]);
//         primes(in_data[0]);
//     } else {
//         // Root Process: generator
//         close(in_data[0]);
//         for (int x=2; x<=280; x++) write_int(in_data[1], x);
//         close(in_data[1]); // EOF to stage-1
//         wait(0);
//         exit(0);
//     }
// }




//------------------ Solution: Using Global Collector FD ----------------------
// user/primes.c
#include "kernel/types.h"
#include "user/user.h"

/************ File-local helpers *********/
static int read_int(int fd, int *v) {
  int n = read(fd, v, sizeof(*v));
  return n == sizeof(*v) ? 1 : 0;
}

static void write_int(int fd, int v) {
  write(fd, &v, sizeof(v));
}

static void filter_for_p(int leftfd, int rightfd, int p) {
  int x;
  while (read_int(leftfd, &x)) {
    if (x % p != 0) write_int(rightfd, x);
  }
  close(leftfd);
  close(rightfd);
}

/************ Global collector write-end *********/
static int report_wfd = -1;

/************ Sieve stage *********/
void primes(int leftfd) __attribute__((noreturn));

void primes(int leftfd) {
  int p;
  if (!read_int(leftfd, &p)) {
    close(leftfd);
    if (report_wfd >= 0) close(report_wfd);  // this stage is done reporting
    exit(0);
  }

  // Send discovered prime to the collector (root prints later)
  write_int(report_wfd, p);

  int right[2];
  pipe(right);
  int pid = fork();
  if (pid == 0) {
    // child: next stage
    close(right[1]);
    close(leftfd);
    // keep report_wfd open so downstream can report
    primes(right[0]); // no return
  } else {
    // parent: filter for p and feed child, then close our report_wfd
    close(right[0]);
    filter_for_p(leftfd, right[1], p);
    close(report_wfd);  // this stage will report nothing more
    wait(0);
    exit(0);
  }
}

/************ Generator + Collector *********/
int main(void) {
  int in[2], report[2];
  pipe(in);
  pipe(report);

  report_wfd = report[1];  // set global *before* forking the sieve

  int pid = fork();
  if (pid == 0) {
    // child: run sieve chain
    close(in[1]);
    close(report[0]);
    primes(in[0]); // no return
  } else {
    // parent: generator + collector
    close(in[0]);
    close(report[1]);

    for (int x = 2; x <= 280; x++) write_int(in[1], x);
    close(in[1]);  // EOF to stage-1

    // Collect all primes, then print once at the end
    int p, cnt = 0;
    int buf[64];   // π(280) < 64
    while (read(report[0], &p, sizeof(p)) == sizeof(p)) {
      if (cnt < 64) buf[cnt++] = p;
    }
    close(report[0]);

    for (int i = 0; i < cnt; i++)
      printf("prime %d\n", buf[i]);

    wait(0);
    exit(0);
  }
}
