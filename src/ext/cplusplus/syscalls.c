/*
 * Newlib system calls
 */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>


int read (int file, char * ptr, int len) {
    return -1;
}

int lseek (int file, int ptr, int dir) {
    return -1;
}

int write (int file, char * ptr, int len) {
    return -1;
}

int open (const char * path, int flags, ... ) {
    return -1;
}

int close (int file) {
    return -1;
}

void _exit (int n) {
    for (;;);
}

int kill (int n, int m) {
    return -1;
}

int getpid (int n) {
    return 1;
}

caddr_t sbrk (int nbytes) {
    return (caddr_t)0;
}

int fstat (int file, struct stat * st) {
    return -1;
}

int link (void) {
    return -1;
}

int unlink (void) {
    return -1;
}

clock_t times (struct tms * tp) {
    return (clock_t) - 1;
}

int isatty (int fd) {
    return 0;
}
