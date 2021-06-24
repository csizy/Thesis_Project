/**
 * @file        main.c
 * @author      Adam Csizy
 * @date        2021-03-04
 * @version     v1.1.0
 * 
 * @brief       Main program module
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include "com_utils.h"
#include "log_utils.h"
#include "stream_utils.h"

/*
 * Compile like this:
 * 
 * gcc -DCC_DEBUG_MODE -O0 -ggdb -Wall log_utils.c com_utils.c camera_utils.c stream_utils.c main.c -pthread -I/<path_to_repo>/CompanionComputer/includes -o streamerapp `pkg-config --cflags --libs gstreamer-1.0`
 *
 * Launch like this:
 * 
 * ./streamerapp
 * ./streamerapp <GC_IP> <GC_PORT>
 */


#define STR_SYSLOG_PROG_NAME                "DroneVideoStreamer" /**< Program's name in the system logger */


/**
 * @brief		The application's main function.
 * 
 * @details		The application's main function is responsible
 * 				for program module initializations.
 * 
 * @return		Result of execution.
 * 
 * @retval      0 Success
 * @retval		<0 Failure
 */
int main(int argc, char* argv[]) {
    
    NetworkInitContext_T *networkCtx = NULL;

    /* Start program as system daemon */
    #ifndef CC_DEBUG_MODE
    if(daemon(0, 0) < 0) {
        
        /* Failed to create daemon */
        perror("daemon");
        fflush(stderr);
        
        exit(EXIT_FAILURE);
    }
    #endif

    /* Open connection to the system logger */
    openlog(STR_SYSLOG_PROG_NAME, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    /* Log program startup */
    createLogMessage(STR_LOG_MSG_MAIN_PROG_STARTUP, LOG_SVRTY_INF);

    /* Setup network module context */
    networkCtx = (NetworkInitContext_T*)calloc(1, sizeof(NetworkInitContext_T));
    if(networkCtx) {

        if(3 == argc) {

            networkCtx->serverNodeName = argv[1];
            networkCtx->serverServiceName = argv[2];
        }    
    }
    else {

        createLogMessage(STR_LOG_MSG_MAIN_CTX_NET_ALLOC_FAIL, LOG_SVRTY_ERR);
        exit(EXIT_FAILURE);
    }

    /* Initialize and start network module */
    if(initNetworkModule(networkCtx)) {

        createLogMessage(STR_LOG_MSG_MAIN_MOD_NET_INIT_FAIL, LOG_SVRTY_ERR);
        closelog();
        exit(EXIT_FAILURE);
    }

    /* Initialize and start streaming module */
    if(initStreamModule()) {

        createLogMessage(STR_LOG_MSG_MAIN_MOD_STRM_INIT_FAIL, LOG_SVRTY_ERR);
        closelog();
        exit(EXIT_FAILURE);
    }

    while(1) {

        /* Idle in main loop */
        sleep(5);
    }

    /* Close system logger */
    closelog();

    exit(EXIT_SUCCESS);
}
