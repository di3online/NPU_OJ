#include <stdio.h>

#include "../include/common.h"
#include "../include/dataitem.h"
#include "../include/debug.h"
#include "../include/judge.h"

Result
check_normal(Judge *p_jdg, Submission submit, TestCase tc)
{
    Result res(submit);
    
    char command[BUFFER_SIZE];

    snprintf(command, sizeof(command) - 1,
            "diff %s/run/stdout.out %s/%s 2>/dev/null 1>&2",
            p_jdg->get_work_dir(), 
            p_jdg->get_work_dir(),
            tc.tc_output_file.c_str());

    int ret = system(command);

    if (0 == ret) {
        res.res_type = NojRes_CorrectAnswer;
    } else if (1 == ret) {
        snprintf(command, sizeof(command) - 1,
                // diff -b -i
                // -b, ignore changes in the amount of white space
                // -i, ignore case differences in file contents
                "diff %s/run/stdout.out %s/%s -b -i 1>/dev/null 2>&1",
                p_jdg->get_work_dir(),
                p_jdg->get_work_dir(),
                tc.tc_output_file.c_str());
        ret = system(command);

        if (0 == ret) {
            res.res_type = NojRes_PresentationError;
        } else if (1 == ret) {
            res.res_type = NojRes_WrongAnswer;
        } else {
            res.res_type = NojRes_SystemError;
            Log::e(command, "Checker");
        }
    } else {
        res.res_type = NojRes_SystemError;
        Log::e(command, "Checker");
    }

    return res;
}

Result
Checker::check(Judge *p_jdg, Submission submit, TestCase tc)
{
    if (!tc.is_special_judge) {
        return check_normal(p_jdg, submit, tc);
    } else {
        Result res(submit);
        res.res_type = NojRes_SystemError;
        res.res_content = "Check: Check method not found";
        return res;
    }
}

