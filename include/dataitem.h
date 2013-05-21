#ifndef SUBMISSION_INCLUDE
#define SUBMISSION_INCLUDE

#include <string>
#include <vector>
#include <map>
#include <queue>

#include <stdlib.h>
#include <string.h>

#include <mysql/mysql.h>

#include "common.h"

struct Result;
struct TestCase;

class Judge;

class TimeUsage {
private:
    time_t  m_time_size; //ms
    bool    m_is_set;
public:
    TimeUsage():
        m_time_size(0),
        m_is_set(false) {}

//    TimeUsage(time_t value):
//        m_time_size(value),
//        m_is_set(true) {}

    TimeUsage(const TimeUsage &rf) {
        if (this != &rf) {
            this->m_time_size = rf.m_time_size;
            this->m_is_set    = rf.m_is_set;
        }
    }

    void set_tv(const struct timeval &tv) {
        this->m_is_set = true;
        this->m_time_size = tv.tv_sec * 1000 
            + tv.tv_usec / 1000;
    }

    void set_millisecond(time_t size) {
        this->m_is_set = true;
        this->m_time_size = size;
    }

    void set_second(time_t size) {
        this->m_is_set = true;
        this->m_time_size = size * 1000;
    }

    inline
    time_t get_millisecond() {
        return this->m_time_size;
    }

    inline
    time_t get_second() {
        return this->m_time_size / 1000 
            + ((this->m_time_size % 1000) ? 1 : 0); 
    }

    inline
    bool is_set() {
        return this->m_is_set;
    }

    inline 
    bool operator> (const TimeUsage &time) {
        return this->m_time_size > time.m_time_size;
    }

    inline
    bool operator< (const TimeUsage &time) {
        return this->m_time_size < time.m_time_size;
    }

    inline 
    bool operator<= (const TimeUsage &time) {
        return !this->operator>(time);
    }

    inline
    bool operator>= (const TimeUsage &time) {
        return !this->operator<(time);
    }

};

class DateTime {
private:
    time_t  m_date_time;
    bool    m_is_set;
public:

    DateTime():
        m_date_time(0),
        m_is_set(false) {}

    DateTime(const DateTime &rf) {
        if (this != &rf) {
            this->m_date_time = rf.m_date_time;
            this->m_is_set = rf.m_is_set;
        }
    }

};

class MemorySize {
private:
    size_t  m_size;     //Byte
    bool    m_is_set;
public:
    MemorySize():
        m_size(0),
        m_is_set(false) {}

//    MemorySize(size_t value):
//        m_size(value),
//        m_is_set(true) {}

    MemorySize(const MemorySize &rf) {
        if (this != &rf) {
            this->m_size = rf.m_size;
            this->m_is_set = rf.m_is_set;
        }
    }

    void set_Byte(size_t size) {
        this->m_is_set = true;
        this->m_size = size;
    }

    void set_KB(size_t size) {
        this->m_is_set = true;
        this->m_size = size << 10;        
    }

    void set_MB(size_t size) {
        this->m_is_set = true;
        this->m_size = size << 20;
    }

    void set_GB(size_t size) {
        this->m_is_set = true;
        this->m_size = size << 30;
    }

    inline 
    size_t get_Byte() {
        return this->m_size;
    }

    inline
    size_t get_KB() {
        return this->m_size >> 10;
    }

    inline
    size_t get_MB() {
        return this->m_size >> 20;
    }

    inline 
    bool is_set() {
        return this->m_is_set;
    }

    inline
    bool operator< (const MemorySize &rf) {
        return this->m_size < rf.m_size;
    }

    inline 
    bool operator> (const MemorySize &rf) {
        return this->m_size > rf.m_size;
    }

    inline 
    bool operator<= (const MemorySize &rf) {
        return !this->operator>(rf);
    }

    inline 
    bool operator>= (const MemorySize &rf) {
        return !this->operator<(rf);
    }
};

struct ResourceLimit {
    MemorySize          memory_limit;     //KB
    TimeUsage           time_limit;       //ms
    MemorySize          file_limit;       //KB

    char                *work_dir;
    int                 syscall_limits[512];

    ResourceLimit():
        memory_limit(),
        time_limit(),
        file_limit(),
        work_dir(NULL) {}

    ResourceLimit(const ResourceLimit &rl) {
        if (this != &rl) {

            this->work_dir = NULL;
            if (rl.work_dir) 
                this->work_dir = strdup(rl.work_dir);

            this->time_limit   = rl.time_limit;
            this->memory_limit = rl.memory_limit;
            this->file_limit   = rl.file_limit;
            memmove(this->syscall_limits, 
                    rl.syscall_limits, 
                    sizeof(rl.syscall_limits));
        }
    }

    ResourceLimit &operator=(const ResourceLimit &rl) {
        this->time_limit = rl.time_limit;
        this->file_limit = rl.file_limit;
        this->memory_limit = rl.memory_limit;
        if (this->work_dir) {
            free(this->work_dir);
            this->work_dir = NULL;
        }

        if (rl.work_dir) {
            this->work_dir = strdup(rl.work_dir);
        }

        memmove(this->syscall_limits, 
                rl.syscall_limits,
                sizeof(rl.syscall_limits));

        return *this;
    }

    ~ResourceLimit() {
        if (this->work_dir) {
            free(work_dir);
            this->work_dir = NULL;
        } 
    }

    void set_workdir(const char *dir) {
        if (this->work_dir && dir) {
            free(this->work_dir);
            this->work_dir = NULL;
        }

        
        if (dir) 
            this->work_dir = strdup(dir);
    }
};

struct Problem {
    typedef long ID;

    Problem::ID             prob_id;
    std::vector<TestCase>   prob_testcases;
    ResourceLimit           prob_rsc_lim;
    
    void clear_testcases(); 

    void add_testcase(TestCase tc);

    TestCase *fetch_testcase(size_t loc); 
};

struct TestCase {
    typedef long ID;
    TestCase::ID    tc_id;
    Problem::ID     tc_prob_id;
    std::string     tc_input_file;
    std::string     tc_output_file;
    int             is_special_judge;
    std::string     tc_remote_input_file;
    std::string     tc_remote_output_file;

    TestCase(TestCase::ID id, Problem::ID prob_id, 
            std::string input = "", std::string output = ""):
        tc_id(id),
        tc_prob_id(prob_id),
        tc_input_file(input),
        tc_output_file(output) {}

    ~TestCase() {}
};

struct Submission {
    typedef long ID;
    Problem::ID         sbm_prob_id;
    Noj_Language        sbm_lang;
    Submission::ID      sbm_id;
    int                 sbm_judge_set;
    Noj_Judge_Mode      sbm_cur_mode;
    Noj_Result          sbm_res_type;

    std::string         sbm_source_file;
    std::string         sbm_binary_file;

    std::vector<Result> sbm_results;
    Submission(Problem::ID prob_id, Noj_Language lang, int mode,
            std::string source_file, std::string binary_file):
        sbm_prob_id(prob_id),
        sbm_lang(lang),
        sbm_judge_set(mode),
        sbm_cur_mode(NojMode_UnSet),
        sbm_res_type(NojRes_Init),
        sbm_source_file(source_file),
        sbm_binary_file(binary_file) {}
    
    ~Submission() {}

};


struct Result {
    typedef long ID;
    Submission::ID  res_sbm_id;
    Noj_Language    res_lang;
    Noj_Result      res_type;
    Noj_Judge_Mode  res_judge_mode;
    std::string     res_content;
    TimeUsage       res_elapsed_time;
    MemorySize      res_max_memory;
    DateTime        res_judge_time;
    TestCase::ID    res_tc_id;
    std::string     res_output;

    Result(Submission::ID sbm_id, 
            Noj_Language lang, 
            Noj_Judge_Mode judge_mode):
        res_sbm_id(sbm_id),
        res_lang(lang),
        res_type(NojRes_Init),
        res_judge_mode(judge_mode),
        res_content(),
        res_elapsed_time(),
        res_max_memory(),
        res_judge_time() {}


    Result(const Result &res):
        res_sbm_id(res.res_sbm_id),
        res_lang(res.res_lang),
        res_type(res.res_type),
        res_judge_mode(res.res_judge_mode),
        res_content(res.res_content),
        res_elapsed_time(res.res_elapsed_time),
        res_max_memory(res.res_max_memory),
        res_judge_time(res.res_judge_time),
        res_tc_id(res.res_tc_id),
        res_output(res.res_output) {}

    Result(const Submission &submit):
        res_sbm_id(submit.sbm_id),
        res_lang(submit.sbm_lang),
        res_type(NojRes_Init),
        res_judge_mode(NojMode_UnSet),
        res_content(),
        res_elapsed_time(),
        res_max_memory(),
        res_judge_time() {}

    ~Result() {}
      
};

class Compiler {

private:

    static char *get_script_path(Submission submit);

public:
    static Result compile(Submission submit, ResourceLimit rl);

};

class Launcher {

private:
    static Result launch_c_or_cpp(
            Submission submit, ResourceLimit rl);

public:
    static Result launch(
            Submission submit, ResourceLimit rl);
};

class Checker {

public:
    static Result check(Judge *p_jdg, Submission submit, TestCase tc);
};

class DBConnMgr {
private:
    enum DBConState{
        DBCS_Init,
        DBCS_Available,
        DBCS_Used,
        DBCS_Closed  
    };
    static const unsigned int       MAX_CONNECTIONS = 10;
    static unsigned int             MAX_AVAILABLE_CONNECTIONS;
    static MYSQL                    m_Connections[MAX_CONNECTIONS];
    static DBConState               m_Conn_bits[MAX_CONNECTIONS];
public:
    static MYSQL *get_available_connection( );
    static void return_connection( MYSQL *mysql_con);
    static MYSQL *connect_db( MYSQL *mysql_con);
    static void  init( );
    static void  terminate( );
};

enum NetworkMode{
    NetMode_NoMode,
    NetMode_FTP,
    NetMode_SFTP
};

class NetworkManager {
private:
    static NetworkManager *s_instance;
    
    NetworkManager();
    enum NetworkMode m_mode;
    char m_host[64 + 1];
    char m_id_file[128 + 1];
    char m_script_file[128 + 1];
    char m_username[32 + 1];
    char m_passwd[32 + 1];

    Noj_State fetch_file_from_sftp(const char *ftp_loc, 
            const char *local_dir);
    Noj_State fetch_file_from_ftp(const char *ftp_loc, 
            const char *local_dir);
public:
    ~NetworkManager();

    Noj_State fetch_file(const char *server_path, const char *local_dir);
    int init_ftp_setting(const char *ip,
            const char *user, const char *passwd);
    int init_sftp_setting(const char *ip, 
            const char *user, const char *id_file);

    int set_ftp_cmd_script(const char *ftp_script);

    static NetworkManager *get_instance();
};

class JudgeManager {
private:
    static JudgeManager *m_instance;
    char *work_dir;
    char *testcase_dir;
    std::queue<Submission>              jm_sbm_lists;
    std::map<Problem::ID, Problem>      jm_prob_lists;
    std::map<TestCase::ID, TestCase>    jm_tc_lists;

    std::queue<Result>                  jm_res_lists;
    JudgeManager(); 
    
    const char *get_testcase_dir();
public:
    ~JudgeManager();

    static JudgeManager *get_instance();

    const char *    get_main_dir();
    Noj_State       fetch_submission_from_db();
    Noj_State       fetch_problem_from_db(Problem::ID);
    Noj_State       fetch_testcase_from_db(TestCase::ID);
    Noj_State       fetch_testcase_from_ftp( TestCase &tc);

    Noj_State       update_results_to_db();
    
    Submission      get_submission();
    Problem         get_problem(Problem::ID prob_id);
    TestCase        get_testcase(TestCase::ID tc_id);

    Noj_State       add_result_to_list(Result res);


    //TODO: Remove at the release version
    void            add_prob(Problem prob) {
        this->jm_prob_lists.insert(
                std::pair<Problem::ID, Problem>(prob.prob_id, prob));

    }
};


class Judge {
private:
    char *work_dir;

    

public:
    Judge():
        work_dir(NULL) {}

    Judge(const Judge &jdg) {
        if (this != &jdg) {
            if (jdg.work_dir) {
                this->work_dir = strdup(jdg.work_dir);
            }
        }
    }

    ~Judge() {
        if (this->work_dir) {
            free(this->work_dir);
        }
    }

    void judge_sbm(Submission &submit);
    void copy_testcase_input(TestCase tc);
    void init_work_dir();
    void clear_work_dir(); 

    void set_work_dir(const char *dir) {
        if (this->work_dir) {
            free(this->work_dir);
        }

        this->work_dir = strdup(dir);
    }

    const char *get_work_dir() {
        return this->work_dir;
    }
    
};

#endif
