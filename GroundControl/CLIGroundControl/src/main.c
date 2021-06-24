/**
 * @file        main.c
 * @author      Adam Csizy
 * @date        2021-04-13
 * @version     v1.1.0
 * 
 * @brief       Main program
 */


#include <stdlib.h>
#include <syslog.h>

#include "com_utils.h"
#include "log_utils.h"
#include "stream_utils.h"


/*
 * Compile like this:
 * 
 * gcc -DGC_DEBUG_MODE -O0 -ggdb -Wall stream_utils.c log_utils.c com_utils.c main.c -pthread -I/<path_to_repo>/GroundControl/CLIGroundControl/includes -o controlapp `pkg-config --cflags --libs gstreamer-1.0`
 * 
 * Launch like this:
 * 
 * ./controlapp
 */

/*
 * Note: Even though the ground control application runs multiple
 * drone service threads and is capable of handling parallel drone
 * connections, the user command interface is not thread safe as
 * multiple service threads have access to the standard input.
 * 
 * Therefore the ground control application should be used only for
 * testing with a single drone connection. The multithreaded approach
 * rather serves as a 'start point' for implementing a ground control
 * application with a proper UI support.
 */


/* Main program module related macro definitions */

#define STR_SYSLOG_PROG_NAME                "GroudControl" /**< Program's name in the system logger */


/**
 * @brief		The application's main function.
 * 
 * @details		The application's main function is responsible
 * 				for the ground control components' initializations.
 * 
 * @return		Result of execution.
 * 
 * @retval      0 Success
 * @retval		<0 Failure
 */
int main(int argc, char* argv[]) {
    
    /* Open connection to the system logger */
    openlog(STR_SYSLOG_PROG_NAME, LOG_PID | LOG_NDELAY, LOG_USER);

    /* Log program startup */
    createLogMessage(STR_LOG_MSG_MAIN_PROG_STARTUP, LOG_SVRTY_INF);

    /* Initialize and start ground control services */
    if(initGroundControlServices()) {

        createLogMessage(STR_LOG_MSG_MAIN_SERVER_INIT_FAIL, LOG_SVRTY_ERR);
        return EXIT_FAILURE;
    }

    /* Initialize streaming services */
    if(initStreamServices()) {

        createLogMessage(STR_LOG_MSG_MAIN_STREAM_INIT_FAIL, LOG_SVRTY_ERR);
        return EXIT_FAILURE;
    }

    while(1) {

        /* Idle in main thread */
        sleep(5);
    }

    /* Close system logger */
    closelog();

    return EXIT_SUCCESS;
}