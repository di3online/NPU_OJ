#include "../include/common.h"
#include "../include/dataitem.h"

Result
check_normal(Submission submit, TestCase tc)
{
    Result res(submit);




    return res;
}

Result
Checker::check(Submission submit, TestCase tc)
{
    if (!tc.is_special_judge) {
        return check_normal(submit, tc);
    }

    Result res(submit);
    res.res_type = NojRes_SystemError;
    res.res_content = "Check: Check method not found";
    return res;
}
