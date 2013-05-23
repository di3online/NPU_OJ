#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/dataitem.h"
#include "../include/debug.h"
#include "../include/config.h"
#include "../include/db.h"

DBConnMgr *DBConnMgr::s_instance = NULL;

DBConnMgr::DBConnMgr():
    m_host(NULL),
    m_user(NULL),
    m_passwd(NULL),
    m_dbname(NULL)
{
}

DBConnMgr::~DBConnMgr()
{
    free(this->m_host);
    free(this->m_user);
    free(this->m_dbname);
    free(this->m_passwd);
    this->terminate();
}

DBConnMgr *DBConnMgr::get_instance()
{
    if (DBConnMgr::s_instance == NULL) {
        DBConnMgr::s_instance = new DBConnMgr();
    }

    return DBConnMgr::s_instance;
}

MYSQL *DBConnMgr::connect_db( MYSQL *p_mysql_con)
{
    Log::d( "Connecting to db", "DBConnMgr");

    mysql_init( p_mysql_con);

    MYSQL *sock = mysql_real_connect( p_mysql_con, 
            this->m_host, this->m_user, 
            this->m_passwd, this->m_dbname, this->m_port, 
            NULL, 0);

    if ( sock == NULL) {
        Log::e( "Connect to db failed.", "DBConnMgr");
    } else {
        Log::d( "Connect to db successfully", "DBConnMgr");
    }

    return sock;
}

void DBConnMgr::init( )
{
    mysql_library_init(0, NULL, NULL);
    for ( unsigned int i = 0;
        i < DBConnMgr::MAX_CONNECTIONS;
        ++i) {
        DBConnMgr::m_Conn_bits[i] = DBCS_Init;
    }

    this->m_host = 
        ConfigManager::get_instance()->get_string("DB_HOST");
    this->m_user =
        ConfigManager::get_instance()->get_string("DB_USER");
    this->m_dbname =
        ConfigManager::get_instance()->get_string("DB_NAME");
    this->m_passwd = 
        ConfigManager::get_instance()->get_string("DB_PASS");

    this->m_port = (int)
        ConfigManager::get_instance()->get_long("DB_PORT");

    this->MAX_AVAILABLE_CONNECTIONS = (unsigned int )
        ConfigManager::get_instance()->get_ulong("DB_MAX_CONNECTIONS");
    if (this->MAX_AVAILABLE_CONNECTIONS > this->MAX_CONNECTIONS) {
        this->MAX_AVAILABLE_CONNECTIONS = this->MAX_CONNECTIONS;
    }

    pthread_mutex_init(&this->m_lock, NULL);

    sem_init(&this->m_sem, 0, this->MAX_AVAILABLE_CONNECTIONS);

}

void DBConnMgr::terminate( )
{
    sem_destroy(&this->m_sem);
    pthread_mutex_destroy(&this->m_lock);
    mysql_library_end();
}

MYSQL *DBConnMgr::get_available_connection( )
{
    MYSQL *con = NULL;
    sem_wait(&this->m_sem);
    pthread_mutex_lock(&this->m_lock);
    for ( unsigned int i = 0; 
        i < DBConnMgr::MAX_AVAILABLE_CONNECTIONS; 
        ++i) {
        if ( DBConnMgr::m_Conn_bits[i] == DBCS_Available) {
            DBConnMgr::m_Conn_bits[i] = DBCS_Used;
            con = &DBConnMgr::m_Connections[i];
            break;
        }
    }

    if (con == NULL) {
        for ( unsigned int i = 0;
            i < DBConnMgr::MAX_AVAILABLE_CONNECTIONS;
            ++i) {
            if ( DBConnMgr::m_Conn_bits[i] == DBCS_Init) {
                DBConnMgr::connect_db( &DBConnMgr::m_Connections[i]);
                DBConnMgr::m_Conn_bits[i] = DBCS_Used;
                con = &DBConnMgr::m_Connections[i];
                break;
            }
        }
    }

    pthread_mutex_unlock(&this->m_lock);

    return con;
}

void DBConnMgr::release_connection( MYSQL *mysql_con)
{
    pthread_mutex_lock(&this->m_lock);
    for ( unsigned int i = 0; 
            i < DBConnMgr::MAX_AVAILABLE_CONNECTIONS;
            ++i) {
        if ( DBConnMgr::m_Conn_bits[i] == DBCS_Used
                && &DBConnMgr::m_Connections[i] == mysql_con) {
            DBConnMgr::m_Conn_bits[i] = DBCS_Available;
        }
    }

    pthread_mutex_unlock(&this->m_lock);
    sem_post(&this->m_sem);
}
