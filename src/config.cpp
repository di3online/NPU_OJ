#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <regex.h>
#include <pcre.h>
#include <string>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

#include "../include/config.h"
#include "../include/common.h"
#include "../include/debug.h"

static char *
regex_get_value(const char *subject, const char *key, char *dest, size_t *size);

static inline bool NOT_COMMENT(char c)
{
    return (c != '#');  
}


void ConfigManager::test_regex()
{
    const char *subject = "DB_NAME=123\n\nDB_port = \nDB_HOST= 127.0.0.1";
    char pattern[BUFFER_SIZE];
    pattern[BUFFER_SIZE - 1] = '\0';
    snprintf(pattern, sizeof(pattern) - 1, 
            "^\\s*%s\\s*=\\s*(\\S+)\\s*$", "db_port");
    const char *error;
    int erroffset;

    pcre *re = pcre_compile(pattern, 
            PCRE_UTF8 | PCRE_NEWLINE_ANY 
            | PCRE_CASELESS | PCRE_MULTILINE,
            &error, &erroffset, NULL);

    if (re == NULL) {
        fprintf(stderr, "%s\n", error);
        return ;
    }

    int overtor[30];

    int rc = pcre_exec(re, NULL, subject, strlen(subject), 0, 0, 
            overtor, 30);
    
    if (rc < 0) {
        fprintf(stderr, "No match\n");
        return ;
    } else {
        fprintf(stderr, "Match\n");
    }

    //for (int i = 0; i < rc; i++) {
    int i = 1;
        const char *substring_start = subject + overtor[2 * i];
        int substring_len = overtor[2 * i + 1] - overtor[2 * i];
        fprintf(stdout, "%.*s\n",substring_len, substring_start);
    //}
    
}

ConfigManager *ConfigManager::s_instance = new ConfigManager();

ConfigManager::ConfigManager():
    m_buffer(NULL) {
}

ConfigManager::~ConfigManager()
{
    free(this->m_buffer);
}

ConfigManager *ConfigManager::get_instance()
{
    return ConfigManager::s_instance;
}

char *
ConfigManager::load_file(const char *file_name)
{
    FILE *fp = NULL;
    long size = 0;
    char *buf = NULL;
    if ((fp=fopen(file_name, "rb")) == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    buf = (char *) malloc(size + 1);
    buf[size] = '\0';
    if (fread(buf, size, 1, fp) < 1) {
        free(buf);
        buf = NULL;
    }

    fclose(fp);

    this->m_buffer = buf;
    
    return this->m_buffer;
}


static char *
regex_get_value(const char *subject, const char *key, 
        char *dest, size_t *size)
{
    if (key == NULL) {
        return NULL;
    }

    char *ret = NULL;
    pcre *re;
    pcre_extra *re_extra;
    char pattern[BUFFER_SIZE];
    const char *error;
    int erroffset;
    int overtor[9];

    pattern[BUFFER_SIZE - 1] = '\0';
    snprintf(pattern, sizeof(pattern) - 1,
            "^\\s*%s\\s*=\\s*(\\S+)\\s*$", key);
    
    re = pcre_compile(pattern,
        PCRE_UTF8 | PCRE_NEWLINE_ANY 
        | PCRE_CASELESS | PCRE_MULTILINE,
        &error, &erroffset, NULL);
    
    if (re != NULL) {
        
        re_extra = pcre_study(re, 0, &error);
        if (error != NULL) {
            fprintf(stderr, "pcre_study %s\n", error);
        }

        int rc = pcre_exec(re, re_extra, subject, strlen(subject), 
                0, 0, overtor, sizeof(overtor) / sizeof(overtor[0]));

        if (rc >= 0) {
            int index = 1;
            const char *substring_start = subject + overtor[2 * index];
            size_t substring_len = overtor[2 * index + 1] - overtor[2 * index];
            if (dest == NULL) {
                dest = (char *) malloc(substring_len + 1);
                if (size != NULL) {
                    *size = substring_len + 1;
                }
                strncpy(dest, substring_start, substring_len);
                dest[substring_len] = '\0';
                ret = dest;
            } else {
                if (*size >= substring_len + 1) {
                    strncpy(dest, substring_start, substring_len);
                    dest[substring_len] = '\0';
                    ret = dest;
                } else {
                    //dest's size is not enough
                    ret = NULL;
                }
            }


        } else {
            //PCRE exec error
        }

        pcre_free_study(re_extra);
    } else {
        //PCRE compile error
        
    }

    free(re);
    return ret;
}

char *
ConfigManager::get_string(const char *key, char *dest, size_t *size)
{
    char *str = 
        regex_get_value(this->m_buffer, key, dest, size);

    if (str == NULL) {
        char buffer[BUFFER_SIZE];
        buffer[BUFFER_SIZE - 1] = '\0';
        snprintf(buffer, sizeof(buffer) - 1, "Cannot get config: %s", key);
        Log::e(buffer, "ConfigManager");
        Log::e("Please check the /etc/noj.conf. Exit", "ConfigManager");
        exit(EXIT_FAILURE);
    }
    return str;
}

long 
ConfigManager::get_long(const char *key)
{
    char *str = 
        regex_get_value(this->m_buffer, key, NULL, NULL);

    if (str == NULL) {
        char buffer[BUFFER_SIZE];
        buffer[BUFFER_SIZE - 1] = '\0';
        snprintf(buffer, sizeof(buffer) - 1, "Cannot get config: %s", key);
        Log::e(buffer, "ConfigManager");
        Log::e("Please check the /etc/noj.conf. Exit", "ConfigManager");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char *error;
    long num = strtol(str, &error, 10);
    if ((error == str) || 
        ((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))
	    || (errno != 0 && num == 0) ) ){
        char buffer[BUFFER_SIZE];
        buffer[BUFFER_SIZE - 1] = '\0';
        snprintf(buffer, sizeof(buffer) - 1,
                "Cannot convert %s to long. Error: %s", str, error);
        Log::e(buffer, "ConfigManager");
        Log::e("Please check the /etc/noj.conf. Exit", "ConfigManager");
        exit(EXIT_FAILURE);
    }

    free(str);

    return num;
    
}

unsigned long
ConfigManager::get_ulong(const char *key)
{
    char *str = 
        regex_get_value(this->m_buffer, key, NULL, NULL);

    if (str == NULL) {
        char buffer[BUFFER_SIZE];
        buffer[BUFFER_SIZE - 1] = '\0';
        snprintf(buffer, sizeof(buffer) - 1, "Cannot get config: %s", key);
        Log::e(buffer, "ConfigManager");
        Log::e("Please check the /etc/noj.conf. Exit", "ConfigManager");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char *error;
    unsigned long num = strtoul(str, &error, 10);
    if  (((errno == ERANGE && 
                (num == ULONG_MAX)) 
            || (errno != 0 && num == 0)) 
        || (error == str)) {
        char buffer[BUFFER_SIZE];
        buffer[BUFFER_SIZE - 1] = '\0';
        snprintf(buffer, sizeof(buffer) - 1,
                "Cannot convert %s to long. Error: %s", str, error);
        Log::e(buffer, "ConfigManager");
        Log::e("Please check the /etc/noj.conf. Exit", "ConfigManager");
        exit(EXIT_FAILURE);
    }

    free(str);

    return num;
}
