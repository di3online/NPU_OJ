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
    std::queue<WorkTask *>      m_que_tasks;
    std::vector<WorkThread *>   m_vec_workthreads;
    pthread_mutex_t             m_mutex;

    ThreadPoolManager() {
        pthread_mutex_init(&this->m_mutex, NULL);

    }

    ~ThreadPoolManager() {
        for (size_t i = 0; i < m_vec_workthreads.size(); ++i) {
            delete m_vec_workthreads[i];
            m_vec_workthreads[i] = NULL;
        }

        pthread_mutex_destroy(&this->m_mutex);
    }

public:
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
    }

    WorkTask *get_sbm() {
        pthread_mutex_lock(&this->m_mutex);
        
        WorkTask *p_task = this->m_que_tasks.front();
        this->m_que_tasks.pop();

        pthread_mutex_unlock(&this->m_mutex);
        return p_task;
    }

    void add_sbm(WorkTask *p_task) {
        pthread_mutex_lock(&this->m_mutex);
        this->m_que_tasks.push(p_task);
        pthread_mutex_unlock(&this->m_mutex);

        return ;
    }
};
#endif
