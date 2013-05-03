
#include <time.h>
#include "../include/debug.h"


FILE *Log::m_err = NULL;
FILE *Log::m_out = NULL;
pthread_rwlock_t Log::m_lock_err;
pthread_rwlock_t Log::m_lock_out;

void 
Log::d(const char *message, const char *classname)
{
    pthread_rwlock_wrlock(&Log::m_lock_out);
    Log::print_datetime(Log::m_out);
    Log::print_ptid(Log::m_out);
    fprintf(Log::m_out, "Class: %s \n", classname);
    fputs(message, Log::m_out);
    fputs("\n", Log::m_out);
    fflush(Log::m_out);

    pthread_rwlock_unlock(&Log::m_lock_out);
}

void 
Log::e(const char *error, const char *classname)
{
    pthread_rwlock_wrlock(&Log::m_lock_err);
    Log::print_datetime(Log::m_err);
    Log::print_ptid(Log::m_err);
    fprintf(Log::m_err, "Class: %s \n", classname);
    fputs(error, Log::m_err);
    fputs("\n", Log::m_err);
    fflush(Log::m_err);

    pthread_rwlock_wrlock(&Log::m_lock_err);
}

void
Log::w(const char *warning, const char *classname)
{
    Log::e(warning, classname);
}

void 
Log::print_datetime(FILE *fout)
{
    time_t timer = time(NULL);
    struct tm *tblock = localtime(&timer);
    fprintf(fout, "%s", asctime(tblock));
}

inline
void 
Log::print_ptid(FILE *fout)
{
    fprintf(fout, "pthread_t: %lu ", pthread_self());
}

