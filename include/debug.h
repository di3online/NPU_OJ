#ifndef __NOJ_DEBUG_INCLUDE_
#define __NOJ_DEBUG_INCLUDE_



#include <stdio.h>
#include <pthread.h>


class Log {

public:
    static FILE                 *m_err;
    static FILE                 *m_out;
    static pthread_rwlock_t     m_lock_err;
    static pthread_rwlock_t     m_lock_out;

    static void print_datetime(FILE *fout);
    static void print_ptid(FILE *fout);

    static void d(const char *message,  const char *classname = "\0");
    static void e(const char *error,    const char *classname = "\0");
    static void w(const char *warning,  const char *classname = "\0");
    
};
#endif
