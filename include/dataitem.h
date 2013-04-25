#ifndef SUBMISSION_INCLUDE
#define SUBMISSION_INCLUDE

#include <string>
#include <vector>
#include <map>
#include <queue>

#include "common.h"

struct Result;
struct Submission {
    long                sbm_prob_id;
    Noj_Language        sbm_lang;
    long                sbm_id;
    int                 sbm_judge_set;
    Noj_Judge_Mode      sbm_cur_mode;

    std::string         sbm_source_file;
    std::string         sbm_binary_file;

    std::vector<Result> sbm_results;
    Submission(long prob_id, Noj_Language lang, int mode,
            std::string source_file, std::string binary_file):
        sbm_prob_id(prob_id),
        sbm_lang(lang),
        sbm_judge_set(mode),
        sbm_cur_mode(NojMode_UnSet),
        sbm_source_file(source_file),
        sbm_binary_file(binary_file) {}
    
    ~Submission() {}

};

class Time {

};

struct Result {
    long            res_sbm_id;
    Noj_Language    res_lang;
    Noj_Result      res_type;
    Noj_Judge_Mode  res_judge_mode;
    std::string     res_content;
    time_t            res_elapsed_time;
    unsigned long            res_max_memory;
    Time            res_judge_time;

    Result(long sbm_id, Noj_Language lang, Noj_Judge_Mode judge_mode):
        res_sbm_id(sbm_id),
        res_lang(lang),
        res_type(NojRes_Init),
        res_judge_mode(judge_mode),
        res_content(),
        res_elapsed_time(0),
        res_max_memory(0),
        res_judge_time() {}


    Result(const Result &res):
        res_sbm_id(res.res_sbm_id),
        res_lang(res.res_lang),
        res_type(res.res_type),
        res_judge_mode(res.res_judge_mode),
        res_content(res.res_content),
        res_elapsed_time(res.res_elapsed_time),
        res_max_memory(res.res_max_memory),
        res_judge_time() {}

    Result(const Submission &submit):
        res_sbm_id(submit.sbm_id),
        res_lang(submit.sbm_lang),
        res_type(NojRes_Init),
        res_judge_mode(NojMode_UnSet),
        res_content(),
        res_elapsed_time(0),
        res_max_memory(0),
        res_judge_time() {}

    ~Result() {}
      
};


struct TestCase {
    long            tc_prob_id;
    std::string     tc_input_file;
    std::string     tc_output_file;
    int             is_special_judge;

    TestCase(long prob_id, std::string input, std::string output):
        tc_prob_id(prob_id),
        tc_input_file(input),
        tc_output_file(output) {}

    ~TestCase() {}
};

struct ResourceLimit {
    unsigned long       memory_limit;     //KB
    time_t              time_limit;       //ms
    unsigned long       file_limit;       //KB

    const char          *work_dir;
    int                 syscall_limits[512];
};

struct Problem {
    long                    prob_id;
    std::vector<TestCase>   prob_testcases;
    ResourceLimit           prob_rsc_lim;
    
    void clear_testcases() {
        this->prob_testcases.clear();
    }

    void add_testcase(TestCase tc) {
        this->prob_testcases.push_back(tc);
    }

    TestCase *fetch_testcase(size_t loc) {
        if (loc < this->prob_testcases.size()) {
            return &this->prob_testcases.at(loc);
        } else {
            return NULL;
        }
    }
};



class Compiler {

private:

    static char *get_script_path(Submission submit);

public:
    static Result compile(Submission submit, ResourceLimit rl);

};

class Launcher {

private:
    static char *get_script_path(Submission submit);

    static Result launch_c_or_cpp(Submission submit, ResourceLimit rl);

public:
    static Result launch(Submission submit, ResourceLimit rl);
};

class Checker {

public:
    static Result check(Submission submit, TestCase tc);
};

class NetworkManager {

};

class JudgeManager {
private:
    static JudgeManager *m_instance;
    char *work_dir;
    std::queue<Submission>      jm_sbm_lists;
    std::map<long, Problem>     jm_prob_lists;
    std::map<long, TestCase>    jm_tc_lists;

    std::queue<Result>          jm_res_lists;
    JudgeManager(); 

public:
    ~JudgeManager();

    static JudgeManager *get_instance();

    const char *    get_main_dir();
    Noj_State       fetch_submission_from_db();
    Noj_State       fetch_problem_from_db();
    Noj_State       fetch_testcase_from_db();

    Noj_State       update_results_to_db();
    
    Submission      get_submission();
    Problem         get_problem(long prob_id);
    TestCase        get_testcase(long tc_id);

    Noj_State       add_result_to_list(Result res);
};


class Judge {
private:
    //const char *work_dir;

public:
    
};

#endif
