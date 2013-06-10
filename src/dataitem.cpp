/******************************************
 * 
 * Author:  Laishzh
 * File:    dataitem.cpp
 * Email:   laishengzhang(at)gmail.com 
 * 
 *******************************************/

#include "../include/common.h"
#include "../include/dataitem.h"


void 
Problem::clear_testcases() {
    this->prob_testcases.clear();
}

void 
Problem::add_testcase(TestCase tc) {
    this->prob_testcases.push_back(tc);
}

TestCase *
Problem::fetch_testcase(size_t loc) {
    if (loc < this->prob_testcases.size()) {
        return &this->prob_testcases.at(loc);
    } else {
        return NULL;
    }
}

