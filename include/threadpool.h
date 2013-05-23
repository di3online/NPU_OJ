#ifndef __THREAD_POOL_INCLUDE
#define __THREAD_POOL_INCLUDE

#include <queue>
#include <vector>

#include <pthread.h>

#include "../include/common.h"
#include "../include/dataitem.h"

class WorkTask {

public:
    virtual void run() = 0;
};

class WorkThread {
private:
    bool        m_is_terminated;
    pthread_t   m_pt_id;
public:

    WorkThread() {
        this->m_is_terminated = false;
    }

    pthread_t get_pthread_id() {
        return this->m_pt_id;
    }

    static 
    void *
    run_thread_entry(void *p_work_thread) {
        WorkThread *p = (WorkThread *) p_work_thread;
        p->run();
        return p;

    }
    void run();
    bool start();
};

class ThreadPoolManager {
private:
    static ThreadPoolManager    *s_instance;

    bool                        m_shutdown;
    std::queue<WorkTask *>      m_que_tasks;
    std::vector<WorkThread *>   m_vec_workthreads;
    pthread_mutex_t             m_queue_mutex;

    pthread_cond_t              m_queue_ready;

    ThreadPoolManager() {
        pthread_mutex_init(&this->m_queue_mutex, NULL);
        pthread_cond_init(&m_queue_ready, NULL);
        this->m_shutdown = true;
    }


public:
    ~ThreadPoolManager() {
        this->pool_destroy();
    }

    static ThreadPoolManager *get_instance() {
        if (s_instance == NULL) {
            s_instance = new ThreadPoolManager();
        }
        return s_instance;
    }

    void init_workthreads(size_t num_threads) {
        while (m_vec_workthreads.size() < num_threads) {
            WorkThread *new_thread = new WorkThread();
            this->m_vec_workthreads.push_back(new_thread);
            new_thread->start();
        }

        this->m_shutdown = false;
    }

    bool is_shutdown();

    void pool_destroy();

    WorkTask *get_sbm() {
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

    void add_sbm(WorkTask *p_task) {
        pthread_mutex_lock(&this->m_queue_mutex);
        this->m_que_tasks.push(p_task);
        pthread_mutex_unlock(&this->m_queue_mutex);
        pthread_cond_signal(&this->m_queue_ready);

        return ;
    }
};
#endif
