#include <stdlib.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/dataitem.h"


JudgeManager *JudgeManager::m_instance = NULL;

JudgeManager *JudgeManager::get_instance() {
    if (JudgeManager::m_instance == NULL) {
        JudgeManager::m_instance = new JudgeManager();
    }

    return JudgeManager::m_instance;
}

JudgeManager::JudgeManager() {
    this->work_dir = get_current_dir_name();
}

JudgeManager::~JudgeManager() {
    if (this->work_dir) {
        free(this->work_dir);
    }
}

const char *
JudgeManager::get_main_dir() {
    return this->work_dir;
}
