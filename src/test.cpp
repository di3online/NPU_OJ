#include <iostream>
#include <stdio.h>
#include "../include/dataitem.h"
#include "../include/common.h"
#include "../include/config.h"
#include "../include/judge.h"

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

void print_config(const char *key)
{
    char *str = ConfigManager::get_instance()->get_string(key, NULL, NULL);
    fprintf(stdout, 
            "%s: %s\n", 
            key, str);
    free(str);
}

void test_config()
{
    print_config("judges_num");
    print_config("FTP_HOST");
    print_config("SFTP_IDF");
}
