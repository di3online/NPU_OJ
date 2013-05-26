
#include "../include/threadpool.h"
#include "../include/debug.h"

void
WorkThread::run() 
{
    Log::d("Thread starts", "WorkThread");
    while (this->m_is_terminated != true) {
        WorkTask *p_task 
            = ThreadPoolManager::get_instance()->get_task();
        if ( p_task == NULL 
                || ThreadPoolManager::get_instance()->is_shutdown()) {
            Log::d("Thread terminates", "WorkThread");

            if (p_task == NULL) {
                Log::e("Thread exits with getting NULL task", "WorkThread");
            } else if (ThreadPoolManager::get_instance()->is_shutdown()){
                Log::e("Thread exits because pool is shut down", "WorkThread");
                free(p_task);
            }
            pthread_exit(NULL);
        } else {
            if (p_task != NULL) {
                p_task->run();
                free(p_task);
            }
        }
    }

    Log::d("Thread exits with jumping out the loop", "WorkThread");

    return ;
}

bool 
WorkThread::start()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048 * 1024);
    int ret = pthread_create(&this->m_pt_id, &attr, run_thread_entry, this);
    if (0 != ret) {
        Log::e("Create thread error", "WorkThread");
    }

    pthread_attr_destroy(&attr);

    return 0 == ret;
}

ThreadPoolManager *
ThreadPoolManager::s_instance = NULL;

bool ThreadPoolManager::is_shutdown()
{
    return this->m_shutdown;
}

ThreadPoolManager *
ThreadPoolManager::get_instance() {
    if (s_instance == NULL) {
        s_instance = new ThreadPoolManager();
    }
    return s_instance;
}

void 
ThreadPoolManager::init_workthreads(size_t num_threads) {
    this->m_shutdown = false;
    while (m_vec_workthreads.size() < num_threads) {
        WorkThread *new_thread = new WorkThread();
        this->m_vec_workthreads.push_back(new_thread);
        new_thread->start();
    }

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

WorkTask *
ThreadPoolManager::get_task() 
{
    pthread_mutex_lock(&this->m_queue_mutex);

    while (this->m_que_tasks.empty() && !this->m_shutdown) {
        pthread_cond_wait(&this->m_queue_ready, &this->m_queue_mutex);
    }

    if (this->m_shutdown) {
        pthread_mutex_unlock(&this->m_queue_mutex);
        return NULL;
    }
    
    WorkTask *p_task = this->m_que_tasks.front();
    this->m_que_tasks.pop();

    pthread_mutex_unlock(&this->m_queue_mutex);
    return p_task;
}

void 
ThreadPoolManager::add_task(WorkTask *p_task) 
{
    pthread_mutex_lock(&this->m_queue_mutex);
    this->m_que_tasks.push(p_task);
    pthread_mutex_unlock(&this->m_queue_mutex);
    pthread_cond_signal(&this->m_queue_ready);
    return ;
}
