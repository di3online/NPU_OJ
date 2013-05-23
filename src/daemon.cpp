#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "../include/daemon.h"
#include "../include/debug.h"


#define LOCKFILE "/var/run/nojd.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

static 
int
check_daemon_running(int fd)
{
    struct flock fk;
    fk.l_start = 0;
    fk.l_whence = SEEK_SET;
    fk.l_len = 0;
    fk.l_type = F_WRLCK;

    if (fcntl(fd, F_GETLK, &fk) < 0) {
        fprintf(stderr, "Cannot get lock %s: %s\n", LOCKFILE, strerror(errno));
        fflush(stderr);
        exit(1);
    }

    if (fk.l_type != F_UNLCK) {
        fprintf(stderr, "Noj is running with PID = %d \n", fk.l_pid);
        fflush(stderr);
        exit(1);
    }

    return 0;
}

static 
int 
lock_file(int fd)
{
    struct flock fk;
    fk.l_type = F_WRLCK;
    fk.l_start = 0;
    fk.l_whence = SEEK_SET;
    fk.l_len = 0;
    //F_SETLKW will wait but F_SETLK will quit
    return fcntl(fd, F_SETLK, &fk);
}

int daemon_run()
{
    int fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);

    if (fd < 0) {
        fprintf(stderr, "Cannot open %s: %s\n", LOCKFILE, strerror(errno));
        fflush(stderr);
        exit(-1);
    }

    check_daemon_running(fd);

    daemon(0, 1);

    int lock = lock_file(fd);
    if (lock < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            fprintf(stderr, "Cannot lock %s: %s\n", LOCKFILE, strerror(errno));
            fprintf(stderr, "EACCES EAGAIN");
            close(fd);
            return -1;
        }

        fprintf(stderr, "Cannot lock %s: %s\n", LOCKFILE, strerror(errno));
        fprintf(stderr, "quit\n");
        fflush(stderr);

        exit(1);
    }

    ftruncate(fd, 0);
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long) getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}
