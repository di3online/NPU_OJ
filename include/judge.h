
#ifndef __JUDGE_INCLUDED_
#define __JUDGE_INCLUDED_

#include <map>
#include <queue>
#include <semaphore.h>

#include "../include/common.h"
#include "../include/dataitem.h"
#include "../include/threadpool.h"

class JudgeTask : public WorkTask {
    Submission m_submit;
public:
    JudgeTask(Submission submit);
    void run();
};

class Judge {
private:
    char *work_dir;
//    Judge(const Judge &jdg) {
//        if (this != &jdg) {
//            if (jdg.work_dir) {
//                this->work_dir = strdup(jdg.work_dir);
//            }
//        }
//    }

public:
//    Judge():
//        work_dir(NULL) {}

    Judge(unsigned int id);


    ~Judge() {
        if (this->work_dir) {
            free(this->work_dir);
        }
    }

    void judge_sbm(Submission &submit);
    void copy_testcase_input(TestCase tc);
    void init_work_dir();
    void clear_work_dir(); 

    void set_work_dir(const char *dir) {
        if (this->work_dir) {
            free(this->work_dir);
        }
        this->work_dir = strdup(dir);
    }

    const char *get_work_dir() {
        return this->work_dir;
    }
    
};

enum JudgeState {
    JS_Available,
    JS_Used
};

class JudgeManager {
private:
    static JudgeManager *s_instance;
    char *work_dir;
    char *testcase_dir;
    char *src_dir;

    unsigned int                        jm_judges_num;
//    std::queue<Submission>              jm_sbm_lists;
    std::map<Problem::ID, Problem>      jm_prob_lists;
    std::map<TestCase::ID, TestCase>    jm_tc_lists;

    std::queue<Result>                  jm_res_lists;
    Judge **                            jm_judges;
    enum JudgeState *                   jm_judges_flag;

    pthread_mutex_t                     jm_lock_judge;
    sem_t                               jm_sem_judge;

    JudgeManager(); 
    
    const char *get_testcase_dir();
    const char *get_src_dir();
    int init_workdir(const char *path);
public:
    ~JudgeManager();

    static JudgeManager *get_instance();

    const char *    get_main_dir();
    int             init_judges();
    int             destroy_judges();

    void            loop_routine();

    Judge *         get_judge();
    void            release_judge(Judge *jdg);

    Noj_State       fetch_submission_from_db(Submission &submit);
    Noj_State       fetch_problem_from_db(Problem::ID);
    Noj_State       fetch_testcase_from_db(TestCase::ID);
    Noj_State       fetch_testcase_from_ftp( TestCase &tc);

    Noj_State       update_results_to_server();
    
    Submission      get_submission();
    Problem         get_problem(Problem::ID prob_id);
    TestCase        get_testcase(TestCase::ID tc_id);

    Noj_State       add_result_to_list(Result res);


    //TODO: Remove at the release version
    void            add_prob(Problem prob) {
        this->jm_prob_lists.insert(
                std::pair<Problem::ID, Problem>(prob.prob_id, prob));

    }
};



#endif //__JUDGE_INCLUDED_
