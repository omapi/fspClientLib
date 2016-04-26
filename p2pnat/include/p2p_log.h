/***************************************************************
Copyright (C), 2003-2015,NewrockTech. Co.,Ltd
FileName : p2p_log.h
Description:
History: <version > <desc> <author> <time> 
*.Version 0.01
*   add logs ;（nliu@newrocktech, 2015/01/29）
*   
***************************************************************/

#ifndef _P2P_LOG_H
#define _P2P_LOG_H

#include <stdio.h>
#include "stdarg.h"
// #include "lib_list.h"

typedef unsigned int    UINT32;
typedef int             INT32;
typedef unsigned short  UINT16;
typedef short           INT16;
typedef unsigned char   UINT8;
typedef char            INT8;

typedef long            LONG;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef unsigned char   BOOL;
typedef void            VOID;
typedef unsigned char   BOOL8;

#define LOCAL static

#define nMAX_EVENT_LOG        50  // 50K events
#define nMAX_EVENT_LOG_LEVEL  10

typedef enum {
    eLOG_TYPE_NONE,
    eLOG_TYPE_FILE,
    eLOG_TYPE_SCREEN,
    eLOG_TYPE_MAX,
} E_LOG_TYPE;

typedef enum {
    eLOG_DUMP_SCREEN,
    eLOG_DUMP_FILE,
} E_LOG_DUMP;

typedef enum {
    eLOG_COMP_API,
    eLOG_COMP_MSG,
    eLOG_COMP_MAX,
} E_LOG_COMP;

typedef enum {
    eLOG_SEVERITY_TRAP,
    eLOG_SEVERITY_ERROR,
    eLOG_SEVERITY_DEBUG,
    eLOG_SEVERITY_TRACE,

    eLOG_SEVERITY_INFO = 6,
} E_LOG_SEVERITY;

typedef struct {
    char           filename[128];
    FILE          *fd;
    unsigned long  current_index;
    unsigned short file_size;    // K
    unsigned char  num_backup_file;
    unsigned char  flag_to_file;
    unsigned char  flag_rotating;
    unsigned char  flag_logging;
    unsigned short miss_event_counter;
    unsigned long  total_miss_event_counter;
    unsigned char  flag_rotate;
} S_LOG_INFO;

typedef enum {
    eLOG_INFO_DEBUG,
    eLOG_INFO_ERROR,
    eLOG_INFO_MSG,

    eLOG_INFO_MAX
} E_LOG_INFO;

typedef struct {
    S_LOG_INFO  log_info[eLOG_INFO_MAX];
    unsigned char log_type;  // file / ram / remote ...
    char          log_level[eLOG_COMP_MAX];
} S_EVENT_LOG_GLOBAL;

extern S_EVENT_LOG_GLOBAL gEventLogGlobal;

#ifdef __cplusplus
extern "C"{
#endif

// debug log need to have log level
void log_init_new(char *filename);
int logger(const char* file_name, int line_num, const char* format, ... );
int debug_log(int comp, int level, const char* file_name, int line_num, const char* format, ... );
int info_log(int comp, int level, const char* format, ... );    
// always log
int err_log(const char* file_name, int line_num, const char* format, ... );
// can be turn on or off
int msg_log(const char* format, ... ); // mgcp msg
// vlog with vlist
int vdebug_log(int comp, int level, const char* full_name, int line_num, const char* format, const char* out_str, va_list *argList );
int verr_log(const char* file_name, int line_num, const char* format, va_list *argList );
// component - API, MSG 
int set_event_log_level(int component, int level);
int get_event_log_level(int component);    
int set_event_log_type(int log_type);
int show_event_log_config();

#define vDbgBreak( x, y)  err_log( __FUNCTION__, __LINE__, "break: %d, %d", x, y )
#define vSysCrash( x, y)  err_log( __FUNCTION__, __LINE__, "crash: %d, %d", x, y )
#define FUNC_ENT()        func_ent( __FUNCTION__ )
#define FUNC_EXIT(rc)     func_exit( __FUNCTION__, rc )
#define FUNC_MARK(rc)     func_exit( __FUNCTION__, rc )    

void func_ent( const char *func );
void func_exit( const char *func, int rc );
void log_init();
void log_term();

#ifdef __cplusplus
}
#endif

#endif // _LOG_H
