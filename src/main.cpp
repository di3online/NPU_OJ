/******************************************
 * 
 * Author:  Laishzh
 * File:    main.cpp
 * Email:   laishengzhang(at)gmail.com 
 * 
 *******************************************/


#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "../include/debug.h"
#include "../include/common.h"
#include "../include/dataitem.h"
#include "../include/daemon.h"
#include "../include/config.h"
#include "../include/db.h"
#include "../include/judge.h"

extern void test_compile(Result res);
extern void print_res_type(Noj_Result ret);
extern void test_init();
extern void test_launch(Result res);


bool g_is_child = false;

void terminate_NOJ()
{
    delete ConfigManager::get_instance();
    delete DBConnMgr::get_instance();


    if (!g_is_child) {
        Log::d("NOJ is exited", "Main");
    }

    Log::destroy();
}

//void test()
//{
//    test_init();
//    Problem prob = JudgeManager::get_instance()->get_problem(1);
//    std::cout << prob.prob_rsc_lim.memory_limit.get_Byte() << std::endl;
//
//    Submission submit(0, NojLang_C, NojMode_Release, "1001.c", "main");
//    submit.sbm_cur_mode = NojMode_Release;
//
//    Judge jdg;
//    jdg.set_work_dir("/home/lai/judge");
//
//    jdg.init_work_dir();
//    jdg.judge_sbm(submit);
//    jdg.clear_work_dir();
//
//    print_res_type(submit.sbm_res_type);
//
//    for (size_t i = 0; i < submit.sbm_results.size(); ++i) {
//        std::cout << "Result " << i << ":" << std::endl;
//        test_launch(submit.sbm_results[i]);
//    }
//    
//}

int main()
{
    
    if (geteuid() != 0) {
        fprintf(stderr, "NOJ must run as root\n");
        exit(1);
    }

    if (getenv("NOJ_WORKDIR") == NULL) {
        setenv("NOJ_WORKDIR", "/usr/local/noj", 0);
    }

    fprintf(stdout, "%s\n", get_current_dir_name());

#ifdef NDEBUG
    daemon_run();
#endif
    Log::init();
    if (chdir(getenv("NOJ_WORKDIR")) < 0) {
        fprintf(stderr, "chdir error: %s\n", strerror(errno));
        exit(1);
    }

    ConfigManager::get_instance()->load_file("/etc/noj.conf");

    atexit(terminate_NOJ);

    if (setenv("TZ", "GMT-8", 1) == -1) {
        Log::e("Unable to set TZ/n", "Main");
    }    

    DBConnMgr::get_instance()->init();
    NetworkManager::get_instance()->init();
    JudgeManager::get_instance()->init_judges();
    Log::d("NOJ Start", "Main");

    JudgeManager::get_instance()->loop_routine();

    return 0;
}
