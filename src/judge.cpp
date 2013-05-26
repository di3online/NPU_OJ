#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>

#include "../include/common.h"
#include "../include/dataitem.h"
#include "../include/debug.h"
#include "../include/db.h"
#include "../include/judge.h"
#include "../include/config.h"
#include "../include/threadpool.h"

#define DEFAULT_DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP)

JudgeTask::JudgeTask(Submission submit):
    m_submit(submit)
{
}

void
JudgeTask::run()
{
    Judge *p_jdg = JudgeManager::get_instance()->get_judge();
    p_jdg->init_work_dir();
    p_jdg->judge_sbm(this->m_submit);
    p_jdg->clear_work_dir();
    JudgeManager::get_instance()->release_judge(p_jdg);

    //TODO: upload the submission's results to judgemanager
    
    return ;
}

Judge::Judge(unsigned int id)
{

    size_t size = 
        strlen(JudgeManager::get_instance()->get_main_dir()) 
            + strlen("/judge") + 5; 
    
    this->work_dir = (char *) malloc(size);
    
    snprintf(this->work_dir, size - 1, 
            "%s%s%u", 
            JudgeManager::get_instance()->get_main_dir(),
            "/judge", 
            id);

}

void 
Judge::init_work_dir() 
{
    struct passwd *user_info;
    user_info = getpwnam("judge");

    uid_t owner = user_info->pw_uid;
    gid_t group = user_info->pw_gid;

    if (user_info == NULL) {
        Log::e("Cannot find user judge, please check installation", 
                "JudgeManager");
        exit(EXIT_FAILURE);
    }

    DIR *dir = NULL;
    if ((dir = opendir(this->work_dir)) == NULL) {
        int ret = mkdir(this->work_dir, DEFAULT_DIR_MODE);
        if (ret != 0) {
            Log::e("Cannot mkdir workdir", "JudgeManager");
            exit(EXIT_FAILURE);
        }
    } else {
        closedir(dir);
    }

    chown(this->work_dir, owner, group);
}

void
Judge::judge_sbm(Submission &submit) 
{
    submit.sbm_res_type = NojRes_Init;

    JudgeManager *p_manager = JudgeManager::get_instance();
    Problem prob = p_manager->get_problem(submit.sbm_prob_id);

    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path) - 1, 
            "%s/run", this->get_work_dir());

    ResourceLimit rl_compile;
    rl_compile.time_limit.set_second(20);
    rl_compile.file_limit.set_MB(20);
    rl_compile.memory_limit.set_MB(100);
    rl_compile.set_workdir(path);

    Result res = Compiler::compile(submit, rl_compile);

    if (res.res_type != NojRes_CompilePass) {
        submit.sbm_results.push_back(res);
        submit.sbm_res_type = NojRes_CompileError;
    } else {
        //Compile Pass
        submit.sbm_res_type = NojRes_CompilePass;
        ResourceLimit rl_launch = prob.prob_rsc_lim;

        rl_launch.set_workdir(path);

        bool flg_ac = true;

        for (size_t i = 0; i < prob.prob_testcases.size(); ++i) { 

            this->copy_testcase_input(prob.prob_testcases[i]);
            res = Launcher::launch(submit, rl_launch);

            if (res.res_type == NojRes_RunPass) {
                res = Checker::check(this, submit, prob.prob_testcases[i]);
            }

            if (res.res_type != NojRes_CorrectAnswer) {
                flg_ac = false;
            }

            submit.sbm_results.push_back(res);
        }

        if (flg_ac) {
            submit.sbm_res_type = NojRes_Accept;
        } else {
            submit.sbm_res_type = NojRes_NotAccept;
        }

    }

}

void 
Judge::copy_testcase_input(TestCase tc)
{
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path) - 1,
            "cp %s/%s %s/run/stdin.in", 
            this->get_work_dir(), 
            tc.tc_input_file.c_str(),
            this->get_work_dir());
            
    int ret = system(path);
    if (ret != 0) {
        fprintf(stderr, "Judge: copy testcase input file error\n");
    }
}

void
Judge::clear_work_dir() 
{
    char command[BUFFER_SIZE];

    snprintf(command, sizeof(command) - 1, 
            "rm -rf %s/*", this->work_dir);
    
    //TODO
    //int ret = system(command);
    //if (ret != 0) {
    //    printf("Judge: clear_work_dir error\n");
    //}
}

JudgeManager *JudgeManager::s_instance = NULL;

JudgeManager *JudgeManager::get_instance() 
{
    if (JudgeManager::s_instance == NULL) {
        JudgeManager::s_instance = new JudgeManager();
    }

    return JudgeManager::s_instance;
}

JudgeManager::JudgeManager() 
{
    this->work_dir = NULL;
}

JudgeManager::~JudgeManager() 
{
    this->destroy_judges();
    if (this->work_dir) {
        free(this->work_dir);
    }
}

void 
JudgeManager::loop_routine() {

    sem_wait(&jm_sem_judge);
    while (1) {
        Submission submit(-1);

        Noj_State stat = this->fetch_submission_from_db(submit);
        if (stat != Noj_Normal) {
            Log::e("Something wrong occurs when fetch submission from db",
                    "JudgeManager");
        }

        JudgeTask *jdg_task = new JudgeTask(submit);

        ThreadPoolManager::get_instance()->add_task(jdg_task);

        sem_post(&jm_sem_judge);

        int ret = sem_trywait(&jm_sem_judge);
        if (ret == 0) {
            //if there is submission waiting to be judge, continue to judge
            continue;
        } else {
            //else update the result and wait for new submission
            this->update_results_to_server();
            sem_wait(&jm_sem_judge);
        }

    }
    return ;
}

static 
Noj_Language
pick_lang(const char *lang)
{
    if (0 == strcasecmp(lang, "C")) {
        return NojLang_C;
    } else if (0 == strcasecmp(lang, "CPP")) {
        return NojLang_CPP;
    } else if (0 == strcasecmp(lang, "Java")) {
        return NojLang_Java;
    } else if (0 == strcasecmp(lang, "Pascal")) {
        
    }
    return NojLang_Unkown;
}

Noj_State 
JudgeManager::fetch_submission_from_db(Submission &submit)
{
    //TODO
    
    MYSQL *db_con = DBConnMgr::get_instance()->get_available_connection();

    char sqlcmd[BUFFER_SIZE];

    while (1) {
        
        snprintf(sqlcmd, sizeof(sqlcmd), 
                "call fetch_submission()");
        int ret = mysql_real_query(db_con, sqlcmd, strlen(sqlcmd));

        if (ret) {
            Log::e("Fetch submission from db error", "JudgeManager");
            Log::e(mysql_error(db_con), "JudgeManager");
            exit(EXIT_FAILURE);
        } else {
            MYSQL_RES *res = mysql_store_result(db_con);

            if (mysql_num_rows(res) != 0) {
                Log::d("Fetch one new Submission from db", "JudgeManager");
                
                MYSQL_ROW row;

                while ((row = mysql_fetch_row(res))) {
                    //sbm_id, Problem_pbm_id, sbm_path, sbm_lang
                    submit.sbm_id          = atol(row[0]);
                    submit.sbm_prob_id     = atol(row[1]);
                    submit.sbm_lang        = pick_lang(row[3]);
                    
                    const char *ftp_loc = row[2];

                    snprintf(sqlcmd, sizeof(sqlcmd), 
                            "%s/%s", this->get_src_dir(), row[0]);

                    submit.sbm_source_file = sqlcmd;

                    NetworkManager::get_instance()->fetch_file(ftp_loc, sqlcmd);

                    //TODO mode
                    //submit.sbm_cur_mode 
                    submit.sbm_judge_set = NojMode_Release;
                }
                mysql_free_result(res);
            } else {
                mysql_free_result(res);
                usleep(300 * 1000); 
            }

            while (!mysql_next_result(db_con)) {
                res = mysql_store_result(db_con);
                mysql_free_result(res);
            }

        }
    }

    DBConnMgr::get_instance()->release_connection(db_con);
    return Noj_Normal;
}

Noj_State
JudgeManager::update_results_to_server()
{

    //TODO
    return Noj_Normal;
}

const char *
JudgeManager::get_main_dir() 
{
    return this->work_dir;
}

int
JudgeManager::init_workdir(const char *path)
{
    char buf_path[BUFFER_SIZE];
    struct passwd *user_info;
    user_info = getpwnam("noj");
    uid_t owner;
    gid_t group;

    if (user_info == NULL) {
        Log::e("Cannot find user noj, please check installation", 
                "JudgeManager");
        exit(EXIT_FAILURE);
    } else {
        owner = user_info->pw_uid;
        group = user_info->pw_gid;
    }

    snprintf(buf_path, sizeof(buf_path) - 1,
            "%s", path);

    DIR *dir = NULL;
    if ((dir = opendir(buf_path)) == NULL) {
        int ret = mkdir(buf_path, DEFAULT_DIR_MODE);
        if (ret != 0) {
            Log::e("Cannot mkdir workdir", "JudgeManager");
            exit(EXIT_FAILURE);
        }
    } else {
        closedir(dir);
    }

    chown(buf_path, owner, group);

    free(this->work_dir);
    this->work_dir = (char *) malloc(strlen(buf_path) + 1);
    strncpy(this->work_dir, buf_path, strlen(buf_path) + 1);

    snprintf(buf_path, sizeof(buf_path) - 1,
            "%s/data", path);
    if ((dir = opendir(buf_path)) == NULL) {
        int ret = mkdir(buf_path, DEFAULT_DIR_MODE);
        if (ret != 0) {
            Log::e("Cannot mkdir workdir", "JudgeManager");
            exit(EXIT_FAILURE);
        }
    } else {
        closedir(dir);
    }
    chown(buf_path, owner, group);
    
    snprintf(buf_path, sizeof(buf_path) - 1,
            "%s/src", path);
    if ((dir = opendir(buf_path)) == NULL) {
        int ret = mkdir(buf_path, DEFAULT_DIR_MODE);
        if (ret != 0) {
            Log::e("Cannot mkdir workdir", "JudgeManager");
            exit(EXIT_FAILURE);
        }
    } else {
        closedir(dir);
    }
    chown(buf_path, owner, group);

    snprintf(buf_path, sizeof(buf_path) - 1,
            "%s/output", path);
    if ((dir = opendir(buf_path)) == NULL) {
        int ret = mkdir(buf_path, DEFAULT_DIR_MODE);
        if (ret != 0) {
            Log::e("Cannot mkdir workdir", "JudgeManager");
            exit(EXIT_FAILURE);
        }
    } else {
        closedir(dir);
    }
    chown(buf_path, owner, group);
    return 0;
}

int 
JudgeManager::init_judges()
{
    //TODO make it not dead
    this->init_workdir("/home/noj");
    this->jm_judges_num = 
        (unsigned int) ConfigManager::get_instance()->get_ulong("JUDGES_NUM");

    this->jm_judges = (Judge **) malloc(sizeof(Judge *) * this->jm_judges_num);
    this->jm_judges_flag = (enum JudgeState *) 
        malloc(sizeof(enum JudgeState) * this->jm_judges_num);
    
    for (unsigned int i = 0; i < this->jm_judges_num; ++i) {
        this->jm_judges[i] = new Judge(i);
        this->jm_judges_flag[i] = JS_Available;
        this->jm_judges[i]->init_work_dir();
    }

    ThreadPoolManager::get_instance()->init_workthreads(this->jm_judges_num);
    pthread_mutex_init(&this->jm_lock_judge, NULL);
    sem_init(&this->jm_sem_judge, 0, this->jm_judges_num);
    
    return 0;
}

int 
JudgeManager::destroy_judges()
{
    ThreadPoolManager::get_instance()->pool_destroy();
    for (unsigned int i = 0; i < this->jm_judges_num; ++i) {
        delete this->jm_judges[i];
    }

    delete this->jm_judges;
    this->jm_judges = NULL;
    delete this->jm_judges_flag;
    this->jm_judges_flag = NULL;

    pthread_mutex_destroy(&this->jm_lock_judge);
    sem_destroy(&this->jm_sem_judge);

    return 0;
}

Judge *
JudgeManager::get_judge()
{
    sem_wait(&this->jm_sem_judge);
    pthread_mutex_lock(&this->jm_lock_judge);   
    Judge *jdg = NULL;
    for (unsigned int i = 0; i < this->jm_judges_num; ++i) {
        if (this->jm_judges_flag[i] == JS_Available) {
            jdg = this->jm_judges[i];
            this->jm_judges_flag[i] = JS_Used;
            break;
        }
    }
    pthread_mutex_unlock(&this->jm_lock_judge);
    
    return jdg;
}

void
JudgeManager::release_judge(Judge *jdg)
{
    pthread_mutex_lock(&this->jm_lock_judge);
    for (unsigned int i = 0; i < this->jm_judges_num; ++i) {
        if (this->jm_judges[i] == jdg) {
            this->jm_judges_flag[i] = JS_Available;
            break;
        }
    }

    pthread_mutex_unlock(&this->jm_lock_judge);
    sem_post(&this->jm_sem_judge);
    return ;
}


Noj_State 
JudgeManager::fetch_problem_from_db( Problem::ID prob_id)
{
    Problem prob;

    //Hint: It's no need to escape the string, because the input is reliable.
    MYSQL *db_con = DBConnMgr::get_instance()->get_available_connection( );
    
    char sqlcmd[BUFFER_SIZE];
    snprintf( sqlcmd, sizeof( sqlcmd), 
            "SELECT pbm_time_limit, pbm_mem_limit FROM Problem "
            "WHERE pbm_id = %ld", prob_id);

    int ret = mysql_real_query( db_con, sqlcmd, strlen( sqlcmd));
    if ( ret ) {
        Log::e( "SQL query failed", "JudgeManager");
        Log::e( sqlcmd, "JudgeManager");
    } else {
        MYSQL_RES *res = mysql_store_result( db_con);
        if ( res) {
            prob.prob_id = prob_id;
            ResourceLimit rl;
            MYSQL_ROW row = mysql_fetch_row( res);
            rl.time_limit.set_millisecond( strtoul( row[0], NULL, 10));
            rl.memory_limit.set_Byte( strtoul( row[1], NULL, 10));
            prob.prob_rsc_lim = rl;

            mysql_free_result( res);
        } else {
            Log::e( "SQL select pbm info but gets nothing", "JudgeManager");
            Log::e( sqlcmd, "JudgeManager");
        }

    }

    snprintf( sqlcmd, sizeof( sqlcmd),
        "SELECT tc_id, tc_input, tc_output FROM TestCase WHERE Problem_pbm_id = %ld",
            prob_id);
    ret = mysql_real_query( db_con, sqlcmd, strlen( sqlcmd));

    if ( ret) {
        Log::e( "SQL query tesecases failed", "JudgeManager");
        Log::e( sqlcmd, "JudgeManager");
    } else {
        MYSQL_RES *res = mysql_store_result( db_con);
        if ( res) {
            MYSQL_ROW row;
            pthread_mutex_lock(&this->jm_lock_judge);
            while ( ( row = mysql_fetch_row( res))) {
                TestCase::ID id = strtoul( row[0], NULL, 10);
                TestCase tc( id, prob_id);
                tc.tc_remote_input_file = row[1];
                tc.tc_remote_output_file = row[2];

                this->fetch_testcase_from_ftp( tc);
                prob.add_testcase(tc);
            }
            this->jm_prob_lists[prob_id] = prob;
            pthread_mutex_unlock(&this->jm_lock_judge);
            mysql_free_result( res);
        } else {
            Log::e( "SQL select testcase info but gets nothing", "JudgeManager");
            Log::e( sqlcmd, "JudgeManager");
        }
    }

    DBConnMgr::get_instance()->release_connection( db_con);
    return Noj_Normal;
    
}


const char *
JudgeManager::get_testcase_dir()
{
    if (testcase_dir == NULL) {
        if (this->work_dir == NULL) {
            return NULL;
        }
        this->testcase_dir = (char *) malloc(strlen(this->work_dir) 
                + strlen("/data") + 2);
        snprintf(this->testcase_dir, 
                strlen(this->work_dir) + strlen("/data") + 1, 
                "%s%s", 
                this->work_dir, "/data");
    }    

    return testcase_dir;
}

const char *
JudgeManager::get_src_dir()
{
    if (this->src_dir == NULL) {
        if (this->work_dir == NULL) {
            return NULL;
        }

        this->src_dir = (char *) malloc(strlen(this->work_dir) 
                + strlen("/src") + 2);
        snprintf(this->src_dir,
                strlen(this->work_dir) + strlen("/src") + 1, 
                "%s%s", 
                this->work_dir, "/src");
    }

    return this->src_dir;
}

Noj_State JudgeManager::fetch_testcase_from_ftp( TestCase &tc)
{
    pthread_mutex_lock(&this->jm_lock_judge);
    Noj_State stat = NetworkManager::get_instance()->fetch_file(
            tc.tc_remote_input_file.c_str(), 
            JudgeManager::get_testcase_dir());

    pthread_mutex_unlock(&this->jm_lock_judge);

    return stat;
}

Problem 
JudgeManager::get_problem(Problem::ID prob_id) 
{
    pthread_mutex_lock(&jm_lock_judge);
    if (this->jm_prob_lists.find(prob_id) 
            == this->jm_prob_lists.end()) {
        pthread_mutex_unlock(&this->jm_lock_judge);
        this->fetch_problem_from_db(prob_id);
    }

    pthread_mutex_lock(&jm_lock_judge);
    Problem prob = this->jm_prob_lists[prob_id];
    pthread_mutex_unlock(&jm_lock_judge);

    return prob;
}

