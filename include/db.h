/******************************************
 * 
 * Author:  Laishzh
 * File:    db.h
 * Email:   laishengzhang(at)gmail.com 
 * 
 *******************************************/

#ifndef __DB_MANAGER_INCLUDED_
#define __DB_MANAGER_INCLUDED_

#include <pthread.h>
#include <semaphore.h>
#include <mysql/mysql.h>

class DBConnMgr {
private:
    enum DBConState{
        DBCS_Init,
        DBCS_Available,
        DBCS_Used,
        DBCS_Closed  
    };

    static DBConnMgr *s_instance;

    DBConnMgr();

    char *m_host;
    char *m_user;
    char *m_passwd;
    char *m_dbname;
    int   m_port;
    static const unsigned int   MAX_CONNECTIONS = 10;
    unsigned int                MAX_AVAILABLE_CONNECTIONS;
    MYSQL                       m_Connections[MAX_CONNECTIONS];
    DBConState                  m_Conn_bits[MAX_CONNECTIONS];

    pthread_mutex_t m_lock;
    sem_t m_sem;
public:

    ~DBConnMgr();

    static DBConnMgr *get_instance();
    MYSQL *get_available_connection( );
    void  release_connection( MYSQL *mysql_con);
    MYSQL *connect_db( MYSQL *mysql_con);
    void  init( );
    void  terminate( );
};
#endif
