
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "../include/debug.h"
#include "../include/common.h"
#include "../include/dataitem.h"

bool g_is_child = false;


void test_compile(Result res)
{
    if (res.res_type == NojRes_CompilePass) {
        std::cout << "Compile Ok" << std::endl;
    } else if (res.res_type == NojRes_CompileError) {
        std::cout << "Compile Error" << std::endl;
        std::cout << res.res_content << std::endl;
        exit(1);
    }
}
void print_res_type(Noj_Result ret) 
{
    switch (ret) {
        case NojRes_Accept:
            std::cout << "Accept";
            break;
        case NojRes_SystemError:
            std::cout << "SystemError";
            break;
        case NojRes_RuntimeError:
            std::cout << "RuntimeError";
            break;
        case NojRes_TimeLimitExceed:
            std::cout << "TimeExceed";
            break;
        case NojRes_MemoryLimitExceed:
            std::cout << "MemoryExceed";
            break;
        case NojRes_OutputLimitExceed:
            std::cout << "OutputExceed";
            break;
        case NojRes_IllegalSystemCall:
            std::cout << "IllegalSystemCall";
            break;
        case NojRes_Abort:
            std::cout << "Abort";
            break;
        case NojRes_SegmentFault:
            std::cout << "SegmentFault";
            break;
        case NojRes_BusError:
            std::cout << "BusError";
            break;
        case NojRes_RunPass:
            std::cout << "RunPass";
            break;
        case NojRes_CompilePass:
            std::cout << "CompilePass";
            break;
        case NojRes_CompileError:
            std::cout << "CompileError";
            break;
        case NojRes_Init:
            std::cout << "Init";
            break;
        case NojRes_PresentationError:
            std::cout << "PresentationError";
            break;
        case NojRes_CorrectAnswer:
            std::cout << "CorrectAnswer";
            break;
        case NojRes_WrongAnswer:
            std::cout << "WrongAnswer";
            break;
        case NojRes_NotAccept:
            std::cout << "NotAccept";
            break;
        default:
            std::cout << "Unkown";
    }

    std::cout << std::endl;
    return ;
}

void test_launch(Result res)
{
    std::cout << "Time:" 
        << res.res_elapsed_time.get_millisecond() 
        << " ms" << std::endl;
    std::cout << "Memory:" << res.res_max_memory.get_MB() << " MB" << std::endl;

    print_res_type(res.res_type);

    std::cout << res.res_content << std::endl;
}

void test_init()
{
    JudgeManager *jm = JudgeManager::get_instance();

    ResourceLimit rl;
    rl.time_limit.set_second(1);
    rl.memory_limit.set_MB(64);
    rl.file_limit.set_MB(2);

    Problem prob;
    prob.prob_id = 0;
    prob.prob_rsc_lim = rl;
    TestCase tc1(0, prob.prob_id, "1.in", "1.out");
    prob.add_testcase(tc1);

    TestCase tc2(1, prob.prob_id, "2.in", "2.out");
    prob.add_testcase(tc2);
    
    jm->add_prob(prob);
}

void terminate_NOJ()
{
    DBConnMgr::terminate( );
    if (!g_is_child) {
        Log::d("NOJ is exited", "Main");
    }
    pthread_rwlock_destroy(&Log::m_lock_err);
    pthread_rwlock_destroy(&Log::m_lock_out);
    fclose(Log::m_out);
    fclose(Log::m_err);
}

int main()
{

    //if (geteuid() != 0) {
    //    fprintf(stderr, "NOJ must run as root\n");
    //    exit(1);
    //}

    FILE *ferr = fopen("nojerr.log", "a+");
    if (ferr == NULL) {
        fprintf(stderr, "Log file: %s/nojerr.log open failed.\n", 
                JudgeManager::get_instance()->get_main_dir());
        exit(1);
    }

    FILE *fout = fopen("nojout.log", "a+");
    if (fout == NULL) {
        fprintf(stdout, "Log file: %s/nojout.log open failed.\n",
                JudgeManager::get_instance()->get_main_dir());
        exit(1);
    }

    Log::m_err = ferr;
    Log::m_out = fout;
    
    pthread_rwlock_init(&Log::m_lock_err, NULL);
    pthread_rwlock_init(&Log::m_lock_out, NULL);

    atexit(terminate_NOJ);

    if (setenv("TZ", "GMT-8", 1) == -1) {
        Log::e("Unable to set TZ/n", "Main");
    }    

    DBConnMgr::init( );
    Log::d("NOJ Start", "Main");

    test_init();
    JudgeManager::get_instance()->fetch_problem_from_db(1);
    Problem prob = JudgeManager::get_instance()->get_problem(1);
    std::cout << prob.prob_rsc_lim.memory_limit.get_Byte() << std::endl;

    Submission submit(0, NojLang_C, NojMode_Release, "1001.c", "main");
    submit.sbm_cur_mode = NojMode_Release;

    Judge jdg;
    jdg.set_work_dir("/home/lai/judge");

    jdg.init_work_dir();
    jdg.judge_sbm(submit);
    jdg.clear_work_dir();

    print_res_type(submit.sbm_res_type);

    for (size_t i = 0; i < submit.sbm_results.size(); ++i) {
        std::cout << "Result " << i << ":" << std::endl;
        test_launch(submit.sbm_results[i]);
    }

    return 0;
}
