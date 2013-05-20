#include <string.h>
#include "../include/dataitem.h"
#include "../include/debug.h"
MYSQL DBConnMgr::m_Connections[DBConnMgr::MAX_CONNECTIONS];
DBConnMgr::DBConState DBConnMgr::m_Conn_bits[DBConnMgr::MAX_CONNECTIONS];
unsigned int DBConnMgr::MAX_AVAILABLE_CONNECTIONS;

MYSQL *DBConnMgr::connect_db( MYSQL *p_mysql_con)
{
    Log::d( "Connecting to db", "DBConnMgr");
    const char *host   = "127.0.0.1";
    const char *user   = "root";
    const char *passwd = "noj";
    const char *db     = "noj";

    mysql_init( p_mysql_con);

    MYSQL *sock = mysql_real_connect( p_mysql_con, 
            host, user, passwd, db, 0, NULL, 0);

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
    DBConnMgr::MAX_AVAILABLE_CONNECTIONS = 3;
    for ( unsigned int i = 0;
        i < DBConnMgr::MAX_CONNECTIONS;
        ++i) {
        DBConnMgr::m_Conn_bits[i] = DBCS_Init;
    }

}

void DBConnMgr::terminate( )
{
    mysql_library_end( );
}

MYSQL *DBConnMgr::get_available_connection( )
{
    for ( unsigned int i = 0; 
        i < DBConnMgr::MAX_AVAILABLE_CONNECTIONS; 
        ++i) {
        if ( DBConnMgr::m_Conn_bits[i] == DBCS_Available) {
            DBConnMgr::m_Conn_bits[i] = DBCS_Used;
            return &m_Connections[i];
        }
    }

    for ( unsigned int i = 0;
        i < DBConnMgr::MAX_AVAILABLE_CONNECTIONS;
        ++i) {
        if ( DBConnMgr::m_Conn_bits[i] == DBCS_Init) {
            DBConnMgr::connect_db( &DBConnMgr::m_Connections[i]);
            DBConnMgr::m_Conn_bits[i] = DBCS_Used;
            return &DBConnMgr::m_Connections[i];
        }
    }

    return NULL;
}

void DBConnMgr::return_connection( MYSQL *mysql_con)
{
    for ( unsigned int i = 0; 
            i < DBConnMgr::MAX_AVAILABLE_CONNECTIONS;
            ++i) {
        if ( DBConnMgr::m_Conn_bits[i] == DBCS_Used
                && &DBConnMgr::m_Connections[i] == mysql_con) {
            DBConnMgr::m_Conn_bits[i] = DBCS_Available;
        }
    }
}
