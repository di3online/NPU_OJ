/******************************************
 * 
 * Author:  Laishzh
 * File:    config.h
 * Email:   laishengzhang(at)gmail.com 
 * 
 *******************************************/

#ifndef __CONFIGMANAGER_INCLUDED_
#define __CONFIGMANAGER_INCLUDED_

#include <stdlib.h>

class ConfigManager {
private:
    static ConfigManager *s_instance;
    char *m_buffer;
    ConfigManager();
public:
    ~ConfigManager();

    static ConfigManager *get_instance();

    void init_config(int argc, char **argv);
    char *load_file(const char *file_name);
    char *get_string(const char *key, char *dest = NULL, size_t *size = NULL);
    unsigned long get_ulong(const char *key);
    long get_long(const char *key);

    char *parse_real_path(const char *path);

    void test_regex();
    
};

#endif//__CONFIGMANAGER_INCLUDED_
