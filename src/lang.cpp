#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include "../include/lang.h"
#include "../include/dataitem.h"
#include "../include/common.h"
#include "../include/debug.h"
#include "../include/judge.h"
#include "../include/config.h"

bool operator==(const LangJudge &a, const LangJudge &b) 
{
    return ((a.get_mode() == b.get_mode()) 
            && (a.get_lang() == b.get_lang()));
}

bool
LangJudge::match(Submission &submit)
{
    return (this->get_lang() == submit.sbm_lang 
            && this->get_mode() == submit.sbm_cur_mode);
}

static
void set_resource_limit(const ResourceLimit &rl)
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

    return ;
}

static 
int RF_C[512] =
{
    SYS_access,             -1,
    SYS_brk,                -1,
    SYS_close,              -1,
    SYS_execve,             1,
    SYS_exit_group,         -1,
    SYS_fstat64,            -1,
    SYS_futex,              -1,
    SYS_gettimeofday,       -1,
    SYS_mmap2,              -1,
    SYS_mremap,             -1,
    SYS_mprotect,           -1,
    SYS_munmap,             -1,
    SYS_lseek,              -1,
    SYS_read,               -1,
    SYS_set_thread_area,    -1,
    SYS_uname,              -1,
    SYS_write,              -1,
    SYS_writev,             -1,
    SYS_time,               -1,
    -1
};

static
void 
init_system_call_limits(ResourceLimit &rl)
{
    //Judge process run
    memset(rl.syscall_limits, 0, sizeof(rl.syscall_limits));

    for (int i = 0; RF_C[i] >= 0; i += 2) {
        rl.syscall_limits[RF_C[i]] = RF_C[i + 1];
    }

    return ;

}

inline time_t convert_timeval_into_ms(struct timeval tv)
{
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static 
long
get_proc_info(pid_t pid, const char *item)
{
    FILE *file;
    char command[BUFFER_SIZE];
    
    sprintf(command, 
            "if [ -d /proc/%d/status ]; then "
            "awk '/%s/ {print $2}' /proc/%d/status 2>/dev/null; "
            "else echo 0; "
            "fi", 
            pid, item, pid);
    file = popen(command, "r");

    if (!file) {
        perror("get_proc_info: popen fail");
    }

    long ret;
    fscanf(file, "%ld", &ret);

    if (file) {
        pclose(file);
    }

    return ret;
}
Result 
LangC::launch(Submission submit, ResourceLimit &rl) 
{
    Result res(submit);


    init_system_call_limits(rl);

    pid_t pid = fork();

    if (pid < 0) {
        res.res_type = NojRes_SystemError;
        res.res_content = "Launch fork error";
    } else if (pid == 0) {

        extern int g_is_child;
        g_is_child = true;

        int ret = chdir(rl.work_dir);
        if (ret != 0) {
            perror(strerror(errno));
        }
        ret = chroot(rl.work_dir);
        if (ret != 0) {
            perror(strerror(errno));
        }

        setuid(rl.rl_uid);
        setresuid(rl.rl_uid, rl.rl_uid, rl.rl_uid);
        
        freopen("stdin.in", "r", stdin);
        freopen("stdout.out", "w", stdout);
        freopen("stderr.out", "a", stderr);

        set_resource_limit(rl);
        nice(19);


        alarm(0);
        alarm(rl.time_limit.get_second() * 5);

        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        /***********************************************
         * Program must compile to static version, 
         * otherwise execl will fail after chroot and chdir
         **********************************************/
        ret = execl("./main", "./main", (char *) NULL);

        if (ret != 0) {
            perror("child launch: execl error");
        }
        exit(1);
    } else {

        int stat;
        struct rusage rs_usg;
        size_t cur_memory_size = 0;

        while (1) {
            wait4(pid, &stat, 0, &rs_usg);
            
            cur_memory_size    = rs_usg.ru_minflt * getpagesize() / 1024;
            res.res_max_memory.set_Byte(
                (res.res_max_memory.get_Byte() < cur_memory_size) ?
                        (cur_memory_size) : (res.res_max_memory.get_Byte()));

            if (rl.memory_limit.is_set() 
                    && res.res_max_memory > rl.memory_limit) {
                res.res_type    = NojRes_MemoryLimitExceed;
                fprintf(stderr, "child killed by mem\n");
                ptrace(PTRACE_KILL, pid, NULL, NULL);
                break;
            }
            
            time_t total_time = 
                convert_timeval_into_ms(rs_usg.ru_utime)
                + convert_timeval_into_ms(rs_usg.ru_stime);
            
            res.res_elapsed_time.set_millisecond(total_time);

            if (rl.time_limit.is_set()
                    && res.res_elapsed_time > rl.time_limit) {
                res.res_type    = NojRes_TimeLimitExceed;

                ptrace(PTRACE_KILL, pid, NULL, NULL);
                break;
            }


            if (WIFEXITED(stat)) {
                if (WEXITSTATUS(stat) == EXIT_SUCCESS) {
                //Exit Normally
                    res.res_type = NojRes_RunPass;
                } else {
                    res.res_type = NojRes_RuntimeError;
                }
                break;
            }

            if (WIFSIGNALED(stat) || 
                    (WIFSTOPPED(stat) && WSTOPSIG(stat) != SIGTRAP)) 
            {
                int signo = ( WIFSIGNALED(stat) ? 
                        (WTERMSIG(stat)) 
                        :(WSTOPSIG(stat)) );

                switch (signo) {
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
            

            struct user_regs_struct reg;
            
            ptrace(PTRACE_GETREGS, pid, NULL, &reg);

            long syscall_id = reg.orig_eax;

            //fprintf(stderr, 
                //"child process made a system call %ld\n", syscall_id);

            if (rl.syscall_limits[syscall_id] >= 0) {
                if (rl.syscall_limits[syscall_id] == 0) {
                    
                    res.res_type = NojRes_IllegalSystemCall;
                    //fprintf(stderr, 
                    //        "child process killed by using syscall:%ld\n", 
                    //        syscall_id);
                    ptrace(PTRACE_KILL, pid, NULL, NULL);

                    break;
                } else {
                    rl.syscall_limits[syscall_id]--;
                }
            } 

            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
        
        cur_memory_size = get_proc_info(pid, "VmPeak:");

        res.res_max_memory.set_Byte(
            (res.res_max_memory.get_Byte() < cur_memory_size) ?
                (cur_memory_size) : (res.res_max_memory.get_Byte()));

        if (rl.memory_limit.is_set()
                && res.res_max_memory > rl.memory_limit) {
            res.res_type = NojRes_MemoryLimitExceed;
        }

    }


    return res;
}

static 
Result
check_normal(Judge *p_jdg, Submission submit, TestCase tc)
{
    Result res(submit);
    
    char command[BUFFER_SIZE];

    snprintf(command, sizeof(command) - 1,
            "diff %s/stdout.out %s 2>/dev/null 1>&2",
            p_jdg->get_work_dir(), 
            tc.tc_output_file.c_str());

    int ret = system(command);

    if (0 == ret) {
        res.res_type = NojRes_CorrectAnswer;
    } else if (1 == WEXITSTATUS(ret)) {
        snprintf(command, sizeof(command) - 1,
                // diff -b -i
                // -b, ignore changes in the amount of white space
                // -i, ignore case differences in file contents
                "diff %s/stdout.out %s -b -i 1>/dev/null 2>&1",
                p_jdg->get_work_dir(),
                tc.tc_output_file.c_str());
        ret = system(command);

        if (0 == ret) {
            res.res_type = NojRes_PresentationError;
        } else if (1 == WEXITSTATUS(ret)) {
            res.res_type = NojRes_WrongAnswer;
        } else {
            res.res_type = NojRes_SystemError;
            Log::e(command, "check_normal");
        }
    } else {
        res.res_type = NojRes_SystemError;
        Log::e(command, "check_normal");
    }

    return res;
}

Result 
LangC::check(Judge *jdg, Submission submit, TestCase tc)
{
    if (!tc.is_special_judge) {
        return check_normal(jdg, submit, tc);
    } else {
        Result res(submit);
        res.res_type = NojRes_SystemError;
        res.res_content = "LangC: Check method not found";
        return res;
    }
    
}

inline
static 
const char *
str_judge_mode(Noj_Judge_Mode mode) 
{
    switch (mode) {
        case NojMode_Debug:
            return "Debug";
        case NojMode_Release:
            return "Release";
        default:
            return "UnSet";
    }
}

Noj_State 
LangC::judge(Judge *jdg, Submission &submit)
{

    submit.sbm_res_type = NojRes_Init;

    jdg->copy_src(submit);

    JudgeManager *p_manager = JudgeManager::get_instance();
    Problem prob = p_manager->get_problem(submit.sbm_prob_id);

    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path) - 1, 
            "%s", jdg->get_work_dir());

    ResourceLimit rl_compile;
    rl_compile.rl_uid = jdg->get_uid();
    rl_compile.time_limit.set_second(20);
    rl_compile.file_limit.set_MB(20);
    rl_compile.memory_limit.set_MB(100);
    rl_compile.set_workdir(path);

    Result res = this->compile(submit, rl_compile);

    if (res.res_type != NojRes_CompilePass) {
        Log::e("Compile fails", "Judge");
        submit.sbm_results.push_back(res);
        submit.sbm_res_type = NojRes_CompileError;
    } else {
        //Compile Pass
        submit.sbm_res_type = NojRes_CompilePass;
        ResourceLimit rl_launch = prob.prob_rsc_lim;
        rl_launch.rl_uid = jdg->get_uid();
        printf("%d\n", rl_launch.rl_uid);
        rl_launch.set_workdir(path);

        rl_launch.set_workdir(path);

        bool flg_ac = true;

        for (size_t i = 0; i < prob.prob_testcases.size(); ++i) { 

            jdg->copy_testcase_input(prob.prob_testcases[i]);
            res = Launcher::launch(submit, rl_launch);
            extern void print_res_type(Noj_Result);
            print_res_type(res.res_type);

            if (res.res_type == NojRes_RunPass) {
                res = this->check(jdg, submit, prob.prob_testcases[i]);
            }

            if (res.res_type != NojRes_CorrectAnswer) {
                flg_ac = false;

                if (res.res_type == NojRes_WrongAnswer 
                        || res.res_type == NojRes_PresentationError) {
                    
                    char buffer[BUFFER_SIZE];
                    snprintf(buffer, sizeof(buffer), 
                            "%s/%ld_%s_%04u.out", 
                            JudgeManager::get_instance()->get_output_dir(),
                            submit.sbm_id, 
                            str_judge_mode(submit.sbm_cur_mode),
                            JudgeManager::get_instance()->get_result_count() 
                                % 100000
                            );
                    res.res_output_file = buffer;
                    jdg->copy_output_file(res);
                } 
            }
            extern void print_res_type(Noj_Result);
            print_res_type(res.res_type);
            submit.sbm_results.push_back(res);
        }

        if (flg_ac) {
            submit.sbm_res_type = NojRes_Accept;
        } else {
            submit.sbm_res_type = NojRes_NotAccept;
        }

    }

    extern void print_res_type(Noj_Result);
    print_res_type(submit.sbm_res_type);

    return Noj_Normal;
}


LangCDebug::LangCDebug()
{
    char *path = 
        ConfigManager::get_instance()->get_string("C_DEBUG_COMPILER");
    char *real_path = 
        ConfigManager::get_instance()->parse_real_path(path);

    if (real_path != NULL) {
        this->compile_script_path = real_path;
        free(path);
    } else {
        this->compile_script_path = path;
    }
}

Noj_Language 
LangCDebug::get_lang() const
{
    return NojLang_C;
}

Noj_Judge_Mode 
LangCDebug::get_mode() const
{
    return NojMode_Debug;
}

Noj_State
LangCDebug::judge(Judge *jdg, Submission &submit)
{
    return LangC::judge(jdg, submit);
}

inline
const char *
LangCDebug::get_script_path()
{
    return this->compile_script_path;
}


Result
LangCDebug::compile(Submission submit, const ResourceLimit &rl)
{
    
    Result res(submit);

    const char *compile_script_path = this->get_script_path();

    fprintf(stdout, "Compile path:%s\n", compile_script_path);

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
        setuid(rl.rl_uid);

        close(fd[0]);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);

        set_resource_limit(rl);

        int ret = execlp(compile_script_path, 
                    compile_script_path,
                    "code", 
                    (void *) NULL);
        if (ret != 0) {
            perror(strerror(errno));
        }

        fprintf(stderr, "No Run");
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


    return res;
}


LangCRelease::LangCRelease()
{
    char *path = 
        ConfigManager::get_instance()->get_string("C_RELEASE_COMPILER");
    char *real_path = 
        ConfigManager::get_instance()->parse_real_path(path);

    if (real_path != NULL) {
        this->compile_script_path = real_path;
        free(path);
    } else {
        this->compile_script_path = path;
    }


}


Noj_Language 
LangCRelease::get_lang() const
{
    return NojLang_C;
}

Noj_Judge_Mode 
LangCRelease::get_mode() const
{
    return NojMode_Release;
}

Noj_State 
LangCRelease::judge(Judge *jdg, Submission &submit)
{
    return LangCDebug::judge(jdg, submit);
}


LangCppDebug::LangCppDebug()
{
    char *path = 
        ConfigManager::get_instance()->get_string("CPP_DEBUG_COMPILER");
    char *real_path = 
        ConfigManager::get_instance()->parse_real_path(path);

    if (real_path != NULL) {
        this->compile_script_path = real_path;
        free(path);
    } else {
        this->compile_script_path = path;
    }
}

Noj_Language 
LangCppDebug::get_lang() const
{
    return NojLang_CPP;
}

Noj_Judge_Mode 
LangCppDebug::get_mode() const
{
    return NojMode_Debug;
}

Noj_State 
LangCppDebug::judge(Judge *jdg, Submission &submit)
{
    return LangCDebug::judge(jdg, submit);
}

LangCppRelease::LangCppRelease()
{
    char *path = 
        ConfigManager::get_instance()->get_string("CPP_RELEASE_COMPILER");
    char *real_path = 
        ConfigManager::get_instance()->parse_real_path(path);

    if (real_path != NULL) {
        this->compile_script_path = real_path;
        free(path);
    } else {
        this->compile_script_path = path;
    }
}

Noj_Language 
LangCppRelease::get_lang() const
{
    return NojLang_CPP;
}

Noj_Judge_Mode 
LangCppRelease::get_mode() const
{
    return NojMode_Release;
}

Noj_State 
LangCppRelease::judge(Judge *jdg, Submission &submit)
{
    return LangCDebug::judge(jdg, submit);
}
