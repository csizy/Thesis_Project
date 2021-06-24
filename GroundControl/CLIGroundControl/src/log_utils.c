/**
 * @file        log_utils.c
 * @author      Adam Csizy
 * @date        2021-04-13
 * @version     v1.1.0
 * 
 * @brief       Logging utilities
 */


#include <stdio.h>
#include <syslog.h>

#include "log_utils.h"


/* Log related function definitions */

void createLogMessage(const char message[], const LogSeverity_T severity) {

    switch(severity) {

        case LOG_SVRTY_ERR:

            syslog(LOG_USER | LOG_ERR, "%s\n", message);
            #ifdef GC_DEBUG_MODE
            fprintf(stdout, "[ERROR] %s\n", message);
            fflush(stdout);
            #endif
            break;

        case LOG_SVRTY_WRN:

            syslog(LOG_USER | LOG_WARNING, "%s\n", message);
            #ifdef GC_DEBUG_MODE
            fprintf(stdout, "[WARNING] %s\n", message);
            fflush(stdout);
            #endif
            break;

        case LOG_SVRTY_INF:

            syslog(LOG_USER | LOG_INFO, "%s\n", message);
            #ifdef GC_DEBUG_MODE
            fprintf(stdout, "[INFORMATION] %s\n", message);
            fflush(stdout);
            #endif
            break;

        case LOG_SVRTY_DBG:

            syslog(LOG_USER | LOG_DEBUG, "%s\n", message);
            #ifdef GC_DEBUG_MODE
            fprintf(stdout, "[DEBUG] %s\n", message);
            fflush(stdout);
            #endif
            break;

        default:

            syslog(LOG_USER | LOG_WARNING, STR_LOG_MSG_UNEXP_SVRTY_LVL);
            #ifdef GC_DEBUG_MODE
            fprintf(stdout, "[WARNING] %s\n", STR_LOG_MSG_UNEXP_SVRTY_LVL);
            fflush(stdout);
            #endif
            break;
    }
}