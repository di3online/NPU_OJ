#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "../include/common.h"
#include "../include/dataitem.h"
#include "../include/debug.h"

JudgeManager *JudgeManager::m_instance = NULL;

JudgeManager *JudgeManager::get_instance() 
{
    if (JudgeManager::m_instance == NULL) {
        JudgeManager::m_instance = new JudgeManager();
    }

    return JudgeManager::m_instance;
}

JudgeManager::JudgeManager() 
{
    this->work_dir = get_current_dir_name();
}

JudgeManager::~JudgeManager() 
{
    if (this->work_dir) {
        free(this->work_dir);
    }
}

const char *
JudgeManager::get_main_dir() 
{
    return this->work_dir;
}

Noj_State JudgeManager::fetch_problem_from_db( Problem::ID prob_id)
{
    Problem prob;

    //Hint: It's no need to escape the string, because the input is reliable.
    MYSQL *db_con = DBConnMgr::get_available_connection( );
    
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
            while ( ( row = mysql_fetch_row( res))) {
                TestCase::ID id = strtoul( row[0], NULL, 10);
                TestCase tc( id, prob_id);
                tc.tc_remote_input_file = row[1];
                tc.tc_remote_output_file = row[2];

                this->fetch_testcase_from_ftp( tc);
                prob.add_testcase(tc);
            }
            this->jm_prob_lists[prob_id] = prob;
            mysql_free_result( res);
        } else {
            Log::e( "SQL select testcase info but gets nothing", "JudgeManager");
            Log::e( sqlcmd, "JudgeManager");
        }
    }

    DBConnMgr::return_connection( db_con);
    return Noj_Normal;
    
}

NetworkManager *NetworkManager::s_instance = NULL;

NetworkManager::NetworkManager():
    m_mode(NetMode_NoMode)
{
    memset(m_host, 0, sizeof(m_host));
    memset(m_id_file, 0, sizeof(m_id_file));
    memset(m_username, 0, sizeof(m_username));
    memset(m_passwd, 0, sizeof(m_passwd));
    memset(m_script_file, 0, sizeof(m_script_file));
}

int NetworkManager::init_ftp_setting(const char *ftp_host, 
        const char *user, const char *passwd)
{
    this->m_mode = NetMode_FTP;
    strncpy(this->m_host, ftp_host, sizeof(this->m_host) - 1);
    strncpy(this->m_username, user, sizeof(this->m_username) - 1);
    strncpy(this->m_passwd, passwd, sizeof(this->m_passwd) - 1);
    return 0;
}

int NetworkManager::init_sftp_setting(const char *sftp_host,
        const char *user, const char *id_file)
{
    this->m_mode = NetMode_SFTP;
    strncpy(this->m_host, sftp_host, sizeof(this->m_host) - 1);
    strncpy(this->m_username, user, sizeof(this->m_username) - 1);
    strncpy(this->m_id_file, id_file, sizeof(this->m_id_file) - 1);
    return 0;
}

int NetworkManager::set_ftp_cmd_script(const char *ftp_script)
{
    if (ftp_script == NULL) {
        //TODO: set default script path
    } else {
        strncpy(this->m_script_file, ftp_script, 
                sizeof(this->m_script_file) - 1);
    }

    return 0;
}


NetworkManager *NetworkManager::get_instance()
{
    if (NetworkManager::s_instance == NULL) {
        NetworkManager::s_instance = new NetworkManager();
    }

    return NetworkManager::s_instance;
}

Noj_State NetworkManager::fetch_file(
        const char *server_path, const char *local_dir)
{
    if (this->m_mode == NetMode_FTP) {
        return this->fetch_file_from_ftp(server_path, local_dir);
    } else if (this->m_mode == NetMode_SFTP){
        return this->fetch_file_from_sftp(server_path, local_dir);
    } else {
        Log::e("Server Mode not match", "NetworkManager");
        return Noj_ServerError;
    }
}


Noj_State NetworkManager::fetch_file_from_ftp(const char *ftp_loc, 
        const char *local_dir)
{
    char ftp_cmd[BUFFER_SIZE];
    ftp_cmd[BUFFER_SIZE - 1] = '\0';
    snprintf(ftp_cmd, sizeof(ftp_cmd) - 1, 
            "su - judge -c \"%s %s %s %s %s %s\" 1>/dev/null 2>&1", 
            this->m_script_file, 
            this->m_host,
            this->m_username, 
            this->m_passwd,
            ftp_loc, 
            local_dir);
    
    int ret = system(ftp_cmd);
    Noj_State stat = Noj_Normal;
    if (0 != ret) {
        switch (ret) {
            case 1:
                stat = Noj_FileNotDownload;
                break;
            default:

                break;
        }
    }

    return stat;
}

Noj_State NetworkManager::fetch_file_from_sftp(const char *ftp_loc, 
        const char *local_dir)
{
    
    char ftp_cmd[BUFFER_SIZE];
    ftp_cmd[BUFFER_SIZE - 1] = '\0';
    snprintf(ftp_cmd, sizeof(ftp_cmd) - 1, 
            "su - judge -c \"%s %s %s %s %s %s\" 1>/dev/null 2>&1", 
            this->m_script_file, 
            this->m_host,
            this->m_username, 
            this->m_id_file,
            ftp_loc, 
            local_dir);
    
    int ret = system(ftp_cmd);
    Noj_State stat = Noj_Normal;
    if (0 != ret) {
        switch (ret) {
            case 1:
                stat = Noj_FileNotDownload;
                break;
            default:
                break;
        }
    }

    return Noj_Normal;
}

const char *JudgeManager::get_testcase_dir()
{
    if (testcase_dir == NULL) {
        if (this->work_dir == NULL) {
            return NULL;
        }
        this->testcase_dir = (char *) malloc(strlen(work_dir) 
                + strlen("/testcases") + 2);
        snprintf(this->testcase_dir, strlen(work_dir) + strlen("/testcases"), 
                "%s/%s", this->work_dir, "/testcases");
    }    

    return NULL;
}

Noj_State JudgeManager::fetch_testcase_from_ftp( TestCase &tc)
{
    Noj_State stat = NetworkManager::get_instance()->fetch_file(
            tc.tc_remote_input_file.c_str(), 
            JudgeManager::get_testcase_dir());

    return stat;
}

Problem 
JudgeManager::get_problem(Problem::ID prob_id) 
{
    return this->jm_prob_lists[prob_id];
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
Judge::init_work_dir() 
{
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
    
    //int ret = system(command);
    //if (ret != 0) {
    //    printf("Judge: clear_work_dir error\n");
    //}
}
