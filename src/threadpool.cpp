
#include "../include/threadpool.h"
#include "../include/debug.h"

void
WorkThread::run() 
{
    Log::d("Thread starts", "WorkThread");
    while (this->m_is_terminated != true) {
        WorkTask *p_task 
            = ThreadPoolManager::get_instance()->get_sbm();
        if ( p_task == NULL 
                && ThreadPoolManager::get_instance()->is_shutdown()) {
            Log::d("Thread terminates", "WorkThread");
            pthread_exit(NULL);
        } else {
            p_task->run();
        }
    }

    return ;
}

bool 
WorkThread::start()
{
    int ret = pthread_create(&this->m_pt_id, NULL, run_thread_entry, this);
    if (0 != ret) {
        Log::e("Create thread error", "WorkThread");
    }

    return 0 == ret;
}

ThreadPoolManager *
ThreadPoolManager::s_instance = NULL;

bool ThreadPoolManager::is_shutdown()
{
    return this->m_shutdown;
}

void ThreadPoolManager::pool_destroy()
{
    if (this->m_shutdown) {
        return ;
    }
    this->m_shutdown = true;

    pthread_cond_broadcast(&this->m_queue_ready);

    for (size_t i = 0; i < m_vec_workthreads.size(); ++i) {
        pthread_join(this->m_vec_workthreads[i]->get_pthread_id(), NULL);
        delete m_vec_workthreads[i];
        m_vec_workthreads[i] = NULL;
    }

}
