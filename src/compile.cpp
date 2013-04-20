#include "../include/common.h"
#include "../include/dataitem.h"

#include <unistd.h>
#include <sys/resource.h>
#include <sys/type.h>
#include <sys/wait.h>

void set_resource_limit(ResourceLimit rl)
{
    struct rlimit lim;
    if (rl.time_limit != 0) {
        lim.rlim_max = rl.time_limit * 3 / 2;
        lim.rlim_cur = rl.time_limit;
        setrlimit(RLIMIT_CPU, &lim);
    }

    if (rl.memory_limit != 0) {
        lim.rlim_max = rl.memory_limit;
        lim.rlim_cur = rl.memory_limit;
        setrlimit(RLIMIT_AS, &lim);
    }

    if (rl.file_limit != 0) {
        lim.rlim_max = rl.file_limit;
        lim.rlim_cur = rl.file_limit;
        setrlimit(RLIMIT_FSIZE, &lim);
    }
    return ;
}

Result 
Compiler::compile(Submission submit, ResourceLimit rl)
{
    
    Result res(submit);

    if (submit.sbm_lang == NojLang_C
            && (submit.sbm_judge_mode & NojMode_Release)) {
        res = Compiler::compile_c_release(submit, rl);
    }

    return res;
}

Result
Compiler::compile_c_release(Submission submit, ResourceLimit rl)
{
    Result res(submit);

    int stat = 0;
    int fd[2];

    char script_file[1024] = "";

    char *dir = get_current_dir_name();

    snprintf(script_file, sizeof(script_file) - 1, 
            "%s/script/C/release/compile.sh", dir);
    free(dir);

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

        close(fd[0]);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);

        set_resource_limit(rl);

        execlp(script_file, submit.sbm_source_file.c_str(), (void *) NULL);

        exit(0);
    } else {
        close(fd[1]);
        char buf[4096];

        waitpid(pid, &stat, 0);

        if (0 == stat) {
            res.res_type = NojRes_CompilePass;            
        } else {
            res.res_type = NojRes_CompileError;
            res.res_content = "";
            while ((size_t size = read(fd[0], buf, sizeof(buf) - 1)) > 0) {
                buf[size] = '\0';
                res.res_content += buf;
            }
        }
    }
    return res;
}
