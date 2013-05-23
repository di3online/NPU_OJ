#include <pwd.h>
#include "../include/debug.h"
#include "../include/dataitem.h"
#include "../include/config.h"


NetworkManager *NetworkManager::s_instance = NULL;

NetworkManager::NetworkManager():
    m_mode(NetMode_NoMode),
    m_port(0)
{
    memset(m_host, 0, sizeof(m_host));
    memset(m_id_file, 0, sizeof(m_id_file));
    memset(m_username, 0, sizeof(m_username));
    memset(m_passwd, 0, sizeof(m_passwd));
    memset(m_down_script, 0, sizeof(m_down_script));
    memset(m_up_script, 0, sizeof(m_up_script));
}

int
NetworkManager::init()
{
    ConfigManager *cf = ConfigManager::get_instance();
    char *mode        = cf->get_string("FTP_MODE");
    char *host;
    int port;
    char *user;
    char *pass;
    char *id_file;
    char *down_script;
    char *up_script;

    if ( 0 == strncasecmp(mode, "sftp", strlen(mode))) {
        this->m_mode = NetMode_SFTP;
        host         = cf->get_string("SFTP_HOST");
        port         = (int) cf->get_long("SFTP_PORT");
        user         = cf->get_string("SFTP_USER");
        id_file      = cf->get_string("SFTP_IDF");
        down_script  = cf->get_string("SFTP_DOWNLOAD_SCRIPT");
        up_script    = cf->get_string("SFTP_UPLOAD_SCRIPT");

        this->init_sftp_setting(host, port, user, id_file);
        this->set_ftp_cmd_script(down_script, up_script);

        free(host);
        free(user);
        free(id_file);
        free(down_script);
        free(up_script);
            
    } else if (0 == strncasecmp(mode, "ftp", strlen(mode))){
        this->m_mode = NetMode_FTP;
        host         = cf->get_string("FTP_HOST");
        port         = (int) cf->get_long("FTP_PORT");
        user         = cf->get_string("FTP_USER");
        pass         = cf->get_string("FTP_PASS");
        down_script  = cf->get_string("FTP_DOWNLOAD_SCRIPT");
        up_script    = cf->get_string("FTP_UPLOAD_SCRIPT");

        this->init_ftp_setting(host, port, user, pass);
        this->set_ftp_cmd_script(down_script, up_script);

        free(host);
        free(user);
        free(pass);
        free(down_script);
        free(up_script);
    } else {
        Log::e("Unkown FTP_MODE, please check /etc/noj.conf", 
                "NetworkManager");       
        exit(EXIT_FAILURE);
    }


    free(mode);

    return 0;
}
int NetworkManager::init_ftp_setting(const char *ftp_host, int port,
        const char *user, const char *passwd)
{
    this->m_mode = NetMode_FTP;
    this->m_port = port;
    strncpy(this->m_host, ftp_host, sizeof(this->m_host) - 1);
    strncpy(this->m_username, user, sizeof(this->m_username) - 1);
    strncpy(this->m_passwd, passwd, sizeof(this->m_passwd) - 1);
    return 0;
}

int NetworkManager::init_sftp_setting(const char *sftp_host, int port, 
        const char *user, const char *id_file)
{
    this->m_mode = NetMode_SFTP;
    this->m_port = port;
    strncpy(this->m_host, sftp_host, sizeof(this->m_host) - 1);
    strncpy(this->m_username, user, sizeof(this->m_username) - 1);
    strncpy(this->m_id_file, id_file, sizeof(this->m_id_file) - 1);
    struct passwd *user_info = getpwnam("noj");
        
    if (user_info == NULL) {
        Log::e("Cannot find user noj, please check installation", 
                "JudgeManager");
        exit(EXIT_FAILURE);
    }

    char cmd[BUFFER_SIZE];
    cmd[BUFFER_SIZE - 1] = '\0';
    snprintf(cmd, sizeof(cmd) - 1,
            "chown noj %s", this->m_id_file);
    int ret = system(cmd);
    if (ret != 0) {
        Log::e("Chown id_file to noj failed", "JudgeManager");
        exit(EXIT_FAILURE);
    } 
    return 0;
}

int NetworkManager::set_ftp_cmd_script(
        const char *down_script, const char *up_script)
{
    strncpy(this->m_down_script, down_script, 
        sizeof(this->m_down_script) - 1);

    strncpy(this->m_up_script, up_script,
        sizeof(this->m_up_script) - 1);
    return 0;
}


NetworkManager *NetworkManager::get_instance()
{
    if (NetworkManager::s_instance == NULL) {
        NetworkManager::s_instance = new NetworkManager();
    }

    return NetworkManager::s_instance;
}

Noj_State NetworkManager::fetch_file(
        const char *server_path, const char *local_dir)
{
    if (this->m_mode == NetMode_FTP) {
        return this->fetch_file_from_ftp(server_path, local_dir);
    } else if (this->m_mode == NetMode_SFTP){
        return this->fetch_file_from_sftp(server_path, local_dir);
    } else {
        Log::e("Server Mode not match", "NetworkManager");
        return Noj_ServerError;
    }
}


Noj_State NetworkManager::fetch_file_from_ftp(const char *ftp_loc, 
        const char *local_dir)
{
    char ftp_cmd[BUFFER_SIZE];
    ftp_cmd[BUFFER_SIZE - 1] = '\0';
    snprintf(ftp_cmd, sizeof(ftp_cmd) - 1, 
            "su - noj -c \"%s %s %s %s %s %s\" 1>/dev/null 2>&1", 
            this->m_down_script, 
            this->m_host,
            this->m_username, 
            this->m_passwd,
            ftp_loc, 
            local_dir);
    
    int ret = system(ftp_cmd);
    Noj_State stat = Noj_Normal;
    if (0 != ret) {
        switch (ret) {
            case 1:
                stat = Noj_FileNotDownload;
                break;
            default:

                break;
        }
    }

    return stat;
}

Noj_State NetworkManager::fetch_file_from_sftp(const char *ftp_loc, 
        const char *local_dir)
{
    
    char ftp_cmd[BUFFER_SIZE];
    ftp_cmd[BUFFER_SIZE - 1] = '\0';
    snprintf(ftp_cmd, sizeof(ftp_cmd) - 1, 
            "su - noj -c \"%s %s %s %s %s %d %s\" 1>/dev/null 2>&1", 
            this->m_down_script, 
            ftp_loc,
            local_dir,
            this->m_username, 
            this->m_host,
            this->m_port,
            this->m_id_file);
    
    int ret = system(ftp_cmd);
    Log::d(ftp_cmd, "NetworkManager");
    Noj_State stat = Noj_Normal;
    if (0 != ret) {
        switch (WEXITSTATUS(ret)) {
            case 1:
                stat = Noj_FileNotDownload;
                Log::e("SFTP file not download", "NetworkManager");
                Log::e(ftp_cmd, "NetworkManager");
                break;
            case 2:
                stat = Noj_PermissionError;
                Log::e("SFTP Permission error", "NetworkManager");
                Log::e(ftp_cmd, "NetworkManager");
                break;
            default:
                Log::e("SFTP download error:Unkown", 
                        "NetworkManager");
                Log::e(ftp_cmd, "NetworkManager");
                break;
        }
    }

    return stat;
}
