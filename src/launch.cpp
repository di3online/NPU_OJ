#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>

#include "../include/common.h"
#include "../include/dataitem.h"


int RF_C[512] =
{
    SYS_access,         -1,
    SYS_brk,            -1,
    SYS_close,          -1,
    SYS_execve,         1,
    SYS_exit_group,     -1,
    SYS_fstat64,        -1,
    SYS_futex,          -1,
    SYS_gettimeofday,   -1,
    SYS_mmap2,          -1,
    SYS_mremap,         -1,
    SYS_mprotect,       -1,
    SYS_munmap,         -1,
    SYS_lseek,          -1,
    SYS_read,           -1,
    SYS_set_thread_area,-1,
    SYS_uname,          -1,
    SYS_write,          -1,
    SYS_writev,         -1,
    SYS_time,           -1,
    -1
};


void set_resource_limit(ResourceLimit rl);

inline time_t convert_timeval_into_ms(struct timeval tv)
{
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


char *
Launcher::get_script_path(Submission submit)
{
    const char *root_dir = JudgeManager::get_instance()->get_main_dir();
    
    const char *file = NULL;

    if (submit.sbm_lang == NojLang_C
            && submit.sbm_cur_mode == NojMode_Release) {
        file = "scripts/launch_c_or_cpp.sh";
    } else {
        return NULL;
    }

    char *script_path = (char *) malloc(strlen(root_dir) + strlen(file) + 2);

    assert(script_path != NULL);
    sprintf(script_path, "%s/%s", root_dir, file);
    //std::cout << script_path << std::endl;

    return script_path;
}

Result 
Launcher::launch(Submission submit, ResourceLimit rl)
{
    if (submit.sbm_lang == NojLang_C
            || submit.sbm_lang == NojLang_CPP) {
        return Launcher::launch_c_or_cpp(submit, rl);
    }

    Result res(submit);
    res.res_type = NojRes_SystemError;
    res.res_content = "Launcher not found";
    return res;
}

static 
long
get_proc_info(pid_t pid, const char *item)
{
    FILE *file;
    char command[BUFFER_SIZE];
    
    sprintf(command, "awk '/%s/ {print $2}' /proc/%d/status 2>/dev/null", item, pid);
    file = popen(command, "r");

    if (!file) {
        perror("get_proc_info: popen fail");
    }

    long ret;
    fscanf(file, "%ld", &ret);

    if (file) {
        fclose(file);
    }

    printf("get_proc_info: %ld\n", ret);

    return ret;
}

static
void 
init_system_call_limits(ResourceLimit &rl)
{
    memset(rl.syscall_limits, 0, sizeof(rl.syscall_limits));

    for (int i = 0; RF_C[i] >= 0; i += 2) {
        rl.syscall_limits[RF_C[i]] = RF_C[i + 1];
    }

    return ;

}

Result 
Launcher::launch_c_or_cpp(Submission submit, ResourceLimit rl)
{
    Result res(submit);

    char *launch_script_path = Launcher::get_script_path(submit);

    fputs(launch_script_path, stdout);
    fputs("\n", stdout);
    fflush(stdout);
    init_system_call_limits(rl);

    if (launch_script_path == NULL) {
        res.res_type = NojRes_SystemError;
        res.res_content = "Launch:: Get launch script fail";
        return res;
    }

    pid_t pid = fork();

    if (pid < 0) {
        res.res_type = NojRes_SystemError;
        res.res_content = "Launch fork error";
    } else if (pid == 0) {

        nice(19);

        chdir(rl.work_dir);
        chroot(rl.work_dir);
        
        freopen("stdin.in", "r", stdin);
        //freopen("stdout.out", "w", stdout);
        freopen("stderr.out", "w", stderr);

        //while(setgid(2000) != 0) sleep(1);
        //while(setuid(2000) != 0) sleep(1);
        //while(setresuid(2000, 2000, 2000) != 0) sleep(1);

        set_resource_limit(rl);

        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        alarm(0);
        alarm(rl.time_limit / 1000 * 10 + 1 );

        fputs(launch_script_path, stdout);
        fputs("\n", stdout);
        fflush(stdout);

       // int ret = execl(launch_script_path, 
       //                 launch_script_path,
       //                 (char *) NULL);
        int ret = execl("./main", "main", (char *) NULL);

        if (ret != 0) {
            perror("child launch: execl error");
        }
        exit(0);
    } else {

        int stat, exit_code;
        struct rusage rs_usg;
        unsigned long cur_memory_size = 0;

        while (1) {
            wait4(pid, &stat, 0, &rs_usg);
            
            cur_memory_size = rs_usg.ru_minflt * getpagesize() / 1024;

            res.res_max_memory = ((cur_memory_size > res.res_max_memory) ? 
                                    (cur_memory_size) : (res.res_max_memory));
            if (res.res_max_memory > rl.memory_limit) {
                res.res_type = NojRes_MemoryLimitExceed;
                cur_memory_size = get_proc_info(pid, "VmPeak:");
                fprintf(stderr, "child killed by mem\n");
                ptrace(PTRACE_KILL, pid, NULL, NULL);
            }
            
            time_t total_time = 
                convert_timeval_into_ms(rs_usg.ru_utime)
                + convert_timeval_into_ms(rs_usg.ru_stime);
            
            res.res_elapsed_time = total_time;

            if (res.res_elapsed_time > rl.time_limit) {
                res.res_type = NojRes_TimeLimitExceed;
                
                cur_memory_size = get_proc_info(pid, "VmPeak:");
                ptrace(PTRACE_KILL, pid, NULL, NULL);
                break;
            }


//            cur_memory_size = get_proc_info(pid, "VmPeak:");
//            
//
//            if (cur_memory_size > res.res_max_memory) {
//                res.res_max_memory = cur_memory_size;
//            }
//
//            if (rl.memory_limit 
//                    && res.res_max_memory > rl.memory_limit) {
//                res.res_type = NojRes_MemoryLimitExceed;
//                fputs("Kill child\n", stdout);
//                ptrace(PTRACE_KILL, pid, NULL, NULL);
//                break;
//            }

            
            if (WIFEXITED(stat)) {
                //Exit Normally
                res.res_type = NojRes_RunPass;
                break;
            }

            if (!(WEXITSTATUS(stat) == 0x05 || WEXITSTATUS(stat) == 0x00)) {
                
                exit_code = WEXITSTATUS(stat);

                switch(exit_code) {
                    case SIGCHLD:
                    case SIGALRM:
                    case SIGKILL:
                    case SIGXCPU:
                        res.res_type = NojRes_TimeLimitExceed;
                        break;
                    case SIGXFSZ:
                        res.res_type = NojRes_OutputLimitExceed;
                        break;

                    case SIGSEGV:
                        res.res_type = NojRes_SegmentFault;
                        break;
                    case SIGBUS:
                        res.res_type = NojRes_BusError;
                        break;
                    case SIGABRT:
                        res.res_type = NojRes_Abort;
                        break;
                    default:
                        res.res_type = NojRes_RuntimeError;
                }

                fputs("Kill child\n", stdout);

                ptrace(PTRACE_KILL, pid, NULL, NULL);
                break;
            }

            
            if (WIFSIGNALED(stat)) {
                int sig = WTERMSIG(stat);

                switch (sig) {
                    case SIGCHLD:
                    case SIGALRM:
                    case SIGKILL:
                    case SIGXCPU:
                        res.res_type = NojRes_TimeLimitExceed;
                        break;
                    case SIGXFSZ:
                        res.res_type = NojRes_OutputLimitExceed;
                        break;

                    case SIGSEGV:
                        res.res_type = NojRes_SegmentFault;
                        break;
                    case SIGBUS:
                        res.res_type = NojRes_BusError;
                        break;
                    case SIGABRT:
                        res.res_type = NojRes_Abort;
                        break;
                    default:
                        res.res_type = NojRes_RuntimeError;
                }
                
                fprintf(stderr, "%s\n", strsignal(sig));
                break;
            }

            struct user_regs_struct reg;
            
            ptrace(PTRACE_GETREGS, pid, NULL, &reg);

            long syscall_id = reg.orig_eax;

            //fprintf(stderr, "child process made a system call %ld\n", syscall_id);

            if (rl.syscall_limits[syscall_id] >= 0) {
                if (rl.syscall_limits[syscall_id] == 0) {
                    
                    cur_memory_size = get_proc_info(pid, "VmPeak:");
                    res.res_type = NojRes_IllegalSystemCall;
                    fprintf(stderr, "child process killed by using syscall:%ld\n", syscall_id);
                    ptrace(PTRACE_KILL, pid, NULL, NULL);

                    break;
                } else {
                    rl.syscall_limits[syscall_id]--;
                }
            } 

            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
        

        if (cur_memory_size > res.res_max_memory) {
            res.res_max_memory = cur_memory_size;
        }

        if (rl.memory_limit 
                && res.res_max_memory > rl.memory_limit) {
            res.res_type = NojRes_MemoryLimitExceed;
        }

    }

    return res;
}
