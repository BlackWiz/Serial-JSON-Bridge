#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>


// Forward declarations
int _close(int fd);
void _exit(int status);
int _fstat(int fd, struct stat *st);
int _isatty(int fd);
int _lseek(int fd, int ptr, int dir);
int _read(int fd, char *ptr, int len);
int _write(int fd, char *ptr, int len);
void* _sbrk(int incr);
int _getpid(void);
int _kill(int pid, int sig);


// These functions are the minimal implementations required for the C library.
// In a real application, you might redirect _write to a UART, for example.

void _exit(int status) {
    // This function is called when the program exits.
    // In a bare-metal system, we just loop forever.
    while (1) {
        ;
    }
}

int _close(int fd) {
    return -1;
}

int _fstat(int fd, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd) {
    return 1;
}

int _lseek(int fd, int ptr, int dir) {
    return 0;
}

int _read(int fd, char *ptr, int len) {
    return 0;
}

int _write(int fd, char *ptr, int len) {
    // A real implementation could write to a UART here.
    return len;
}

int _getpid(void) {
    // Return a dummy process ID
    return 1;
}

int _kill(int pid, int sig) {
    // We don't have processes, so return an error
    errno = EINVAL;
    return -1;
}

void* _sbrk(int incr) {
    extern char _end; // Symbol defined by the linker script
    static char *heap_end;
  //  char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }
  //  prev_heap_end = heap_end;

    // We don't have a real heap, so we just return an out-of-memory error.
    errno = ENOMEM;
    return (void *)-1;
}