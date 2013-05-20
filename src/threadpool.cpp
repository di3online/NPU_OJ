#include "../include/threadpool.h"
#include "../include/debug.h"

void
WorkThread::run() 
{
    Log::d("Thread starts", "WorkThread");
    while (this->m_is_terminated != true) {
        WorkTask *p_task 
            = ThreadPoolManager::get_instance()->get_sbm();
        p_task->run(); 
    }

    Log::d("Thread terminates", "WorkThread");
    pthread_exit(NULL);
    return ;
}

bool 
WorkThread::start()
{
    int ret = pthread_create(&this->m_pt_id, NULL, run_thread_entry, this);
    if (0 == ret) {
        Log::e("Create thread error", "WorkThread");
    }

    return 0 == ret;
}

ThreadPoolManager *
ThreadPoolManager::s_instance = NULL;
