#include <iostream>
#include <assert.h>
#include <stdlib.h>

#include "../include/common.h"
#include "../include/dataitem.h"


#include <sys/syscall.h>

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

void test_launch(Result res)
{
    std::cout << "Time:" << res.res_elapsed_time << std::endl;
    std::cout << "Memory:" << res.res_max_memory << std::endl;
    if (res.res_type == NojRes_RunPass) {
        std::cout << "Run Ok" << std::endl;
    } else {
        std::cout << "Run fail" << std::endl;
        switch (res.res_type) {
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
            default:
                std::cout << "Unkown";
        }

        std::cout << std::endl;
        std::cout << res.res_content << std::endl;
    }
}

int main()
{
    //JudgeManager *jm = JudgeManager::get_instance();
    
    ResourceLimit rl;
    rl.time_limit   = 0;
    rl.memory_limit = 0;
    rl.file_limit   = 0;

    Submission submit(1, NojLang_C, NojMode_Release, "1001.c", "main");
    submit.sbm_cur_mode = NojMode_Release;

    Result res = Compiler::compile(submit, rl);

    test_compile(res);

    rl.time_limit   = 10000;
    rl.memory_limit = 64 * 1024;
    rl.file_limit   = 64 * 1024;
    rl.work_dir     = JudgeManager::get_instance()->get_main_dir();
    res             = Launcher::launch(submit, rl);

    test_launch(res);


    return 0;
}
