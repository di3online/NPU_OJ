#ifndef __DAEMON_INCLUDED
#define __DAEMON_INCLUDED

#include <fcntl.h>

//return positive, if another daemon is running
//return 0, daemon init successfully
//return -1, EACCES or EAGAIN occured
int daemon_run();
#endif
