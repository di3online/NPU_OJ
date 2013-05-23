#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>

#include "../include/common.h"
#include "../include/dataitem.h"
#include "../include/judge.h"


void set_resource_limit(ResourceLimit rl)
{
    struct rlimit lim;
    if (rl.time_limit.is_set()) {
        lim.rlim_max = rl.time_limit.get_second() * 3 / 2 + 1;
        lim.rlim_cur = rl.time_limit.get_second() + 1;
        setrlimit(RLIMIT_CPU, &lim);
    }

    if (rl.memory_limit.is_set()) {
        lim.rlim_max = rl.memory_limit.get_Byte() + 4 * 1024 * 1024;
        lim.rlim_cur = rl.memory_limit.get_Byte() + 4 * 1024 * 1024;
        setrlimit(RLIMIT_AS, &lim);
    }

    if (rl.file_limit.is_set()) {
        lim.rlim_max = rl.file_limit.get_Byte();
        lim.rlim_cur = rl.file_limit.get_Byte();
        setrlimit(RLIMIT_FSIZE, &lim);
    }

    fflush(stderr);
    return ;
}

char *
Compiler::get_script_path(Submission submit)
{
    const char *root_dir = JudgeManager::get_instance()->get_main_dir();

    Noj_Language lang = submit.sbm_lang;
    Noj_Judge_Mode mode = submit.sbm_cur_mode;

    const char *file = NULL;

    if (lang == NojLang_C && mode == NojMode_Release) {
        file = "scripts/compile_c_release.sh";
    } else {
        return NULL;
    }

    char *script_path = (char *) malloc(strlen(root_dir) + strlen(file) + 3);

    assert(script_path != NULL);
    sprintf(script_path, "%s/%s", root_dir, file);

    return script_path;
}

Result 
Compiler::compile(Submission submit, ResourceLimit rl)
{
    
    Result res(submit);

    char *compile_script_path = Compiler::get_script_path(submit);

    if (compile_script_path == NULL) {
        res.res_type = NojRes_SystemError;
        res.res_content = "Compile: Get compile script fail";
        return res;
    }

    int stat = 0;
    int fd[2];

    if (pipe(fd) != 0) {
        res.res_type = NojRes_SystemError;
        res.res_content = "Compile: Create pipe error";
        return res;
    }
    
    pid_t pid = fork();

    if (pid < 0) {
        res.res_type = NojRes_SystemError;
        res.res_content = "Compile fork error";
    } else if (pid == 0) {
        //Child progress
        extern bool g_is_child;
        g_is_child = true;
        chdir(rl.work_dir);

        close(fd[0]);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);

        set_resource_limit(rl);

        execl(compile_script_path, 
                compile_script_path,
                submit.sbm_source_file.c_str(), 
                (void *) NULL);

        exit(1);

    } else {

        close(fd[1]);
        char buf[4096];

        waitpid(pid, &stat, 0);

        if (0 == stat) {
            res.res_type = NojRes_CompilePass;            
        } else {
            res.res_type = NojRes_CompileError;
            res.res_content = "";
            size_t size;
            while ((size = read(fd[0], buf, sizeof(buf) - 1)) > 0) {
                buf[size] = '\0';
                res.res_content += buf;
            }
        }
    }


    if (compile_script_path != NULL) {
        free(compile_script_path);
    }

    return res;
}

