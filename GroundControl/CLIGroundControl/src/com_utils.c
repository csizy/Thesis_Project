/**
 * @file        com_utils.c
 * @author      Adam Csizy
 * @date        2021-04-13
 * @version     v1.1.0
 * 
 * @brief       Ground Control communication utilities
 */


#include <gst/gst.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "com_utils.h"
#include "log_utils.h"
#include "stream_utils.h"


/* Communication related macro definitions */

#define MessageHeaderField_T uint32_t   /**< Type of the fields in the header of network messages */
#define LoginMessageField_T uint32_t    /**< Type of the fields in the login netork message */

#define SOCK_FD_INVAL -1                /**< Invalid socket file descriptor */
#define NUM_SERVER_PORT 5010            /**< Port number of TCP server */
#define NUM_SERVER_PEND_QUEUE_LIMIT 16  /**< Maximal number of pending connections */
#define NUM_DRONE_SRVC_THRD_POOL_SIZE 1 /**< Thread pool size of drone service threads */
#define NUM_MSG_HEADER_SIZE 2U          /**< Size of message header array in MessageHeaderField_T */
#define IDX_MSG_HEADER_MODULE 0U        /**< Index of module name in message header array */
#define IDX_MSG_HEADER_CODE 1U          /**< Index of module message code in message header array */
#define NUM_LOGIN_MSG_SIZE 2U           /**< Size of login message array in LoginMessageField_T */
#define IDX_LOGIN_MSG_CODE 0U           /**< Index of module message code in login message array */
#define IDX_LOGIN_MSG_ID 1U             /**< Index of drone ID in login message array */
#define NUM_POLL_ARR_SIZE 2U            /**< Size of poll array */
#define IDX_POLL_ARR_SOCK 1U            /**< Index of socket element in poll array */
#define IDX_POLL_ARR_CLI 0U             /**< Index of CLI element in poll array */
#define NUM_MAX_CMD_ARGS 1U             /**< Maximal number of user command arguments including the command itself */
#define NUM_CMD_BUFF_SIZE 64U           /**< Size of the user command buffer in bytes */

#define STR_USR_CMD_STRM_PLAY   "play"  /**< String of 'play' user command */
#define STR_USR_CMD_STRM_STOP   "stop"  /**< String of 'stop' user command */
#define STR_USR_CMD_DRN_DCON    "dconn" /**< String of 'dconn' user command */


/* Communication related static variable declarations */

static int serverSocketFd = SOCK_FD_INVAL;      /**< File descriptor of the server socket */
static pthread_mutex_t serverSocketLock = PTHREAD_MUTEX_INITIALIZER;    /**< Mutex for locking/protecting the server socket */
static pthread_t droneServiceThreadPool[NUM_DRONE_SRVC_THRD_POOL_SIZE]; /**< Thread pool of drone service threads */


/* Communication related static function declarations */

/**
 * @brief       Start ground control's server.
 * 
 * @details     Configures and starts a server listening on a
 *              TCP port for incoming connection requests sent
 *              from remote drone (UAV) platforms. This function
 *              is only responisble for initializig the server's
 *              socket and not for handling client connections.
 * 
 * @note        The server socket descriptor must be protected with mutex. 
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int startServer(void);

/**
 * @brief       Start drone service threads.
 * 
 * @details     Starts threads which are responsible for handling
 *              incoming connection requests and communication
 *              between the drone and the ground control.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int startDroneServiceThreads(void);

/**
 * @brief       Start routine of drone service thread.
 * 
 * @details     Estabilishes connection with a remote drone and
 *              handles incoming network traffic over IP/TCP.
 *              This function also handles user commands from the
 *              CLI and invokes the appropriate handler functions
 *              to maintain communication between the ground control
 *              and the remote drone.
 * 
 * @param[in]   arg Thread ID.
 * 
 * @return      Any (not used).
 */
static void *threadFuncDroneService(void *arg);

/**
 * @brief       Authenticate drone.
 * 
 * @details     Authenticates remote drone by exchanging
 *              login messages and the drone ID using the
 *              given service socket.
 * 
 * @param[in]   serviceSocket File descriptor of service socket.
 * @param[out]  droneID Drone ID
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int authDrone(const int serviceSocket, LoginMessageField_T *droneID);

/**
 * @brief       Handle input message.
 * 
 * @details     Handles input messages received over IP/TCP
 *              from the drone. Message handling includes
 *              parsing message code and target module name
 *              specified in the message header as well as
 *              calling the corresponding message handlers
 *              per module. On failure the network RX buffer
 *              is cleaned up to preserve consistency. 
 * 
 * @param[in]   serviceSocket File descriptor of service socket.
 * @param[in,out]   pipeline GStreamer video display pipeline.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int inputMessageHandler(const int serviceSocket, GstElement* *pipeline);

/**
 * @brief       Handle input commands.
 * 
 * @details     Handles CLI commands received from the
 *              standard input. This function parses the
 *              input commands and invokes the corresponding
 *              handler functions.
 * 
 * @param[in]   stdinFd File descriptor of the standard input.
 * @param[in,out]   exitCondition Exit condition for the caller thread.
 * @param[in,out]   pipeline GStreamer video display pipeline.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int inputCommandHandler(const int stdinFd, int *exitCondition, GstElement* *pipeline);

/**
 * @brief       Send stream stop message.
 * 
 * @details     Sends stream stop module message to
 *              the drone.
 * 
 * @param[in]   serviceSocket File descriptor of service socket.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int sendStopMessage(const int serviceSocket);

/**
 * @brief       Clean up input messages.
 * 
 * @details     Cleans up input messages available through
 *              the given network socket file descriptor by
 *              reading network RX buffer as long as data is
 *              available.
 *
 * @note        Not thread safe. In case of reading the network
 *              socket from multiple threads simultaneously
 *              mutual exclusion must be applied.
 * 
 * @param[in]   sockFd Network socket file descriptor.
 */
static void cleanupInputMessages(const int sockFd);


/* Communication related function definitions */

ssize_t recvTimeout(int sockfd, void *buf, size_t len, int flags, time_t sec, useconds_t usec) {

    ssize_t retval;
    struct timeval timeout = {

        .tv_sec = sec,
        .tv_usec = usec
    };
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    retval = recv(sockfd, buf, len, flags);
    timeout = (struct timeval){ .tv_sec = 0, .tv_usec = 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return retval;
}

int initGroundControlServices(void)
{

    int retval = 0;

    if (startServer())
    {

        createLogMessage(STR_LOG_MSG_FUNC1_SRVR_START_FAIL, LOG_SVRTY_ERR);
        retval = -1;
        return retval;
    }

    if (startDroneServiceThreads())
    {

        createLogMessage(STR_LOG_MSG_FUNC1_THRD_START_FAIL, LOG_SVRTY_ERR);
        retval = -1;
        return retval;
    }

    return retval;
}

static int startServer(void) {

    int retval = 0;
    int reuseAddrState = 1;
    struct sockaddr_in6 serverAddress;

    /* Create a TCP server socket with IPv6 compatibility */
    serverSocketFd = socket(PF_INET6, SOCK_STREAM, 0);
    if (0 > serverSocketFd) {

        perror("socket");
        fflush(stderr);
        createLogMessage(STR_LOG_MSG_FUNC2_SOCK_CREAT_FAIL, LOG_SVRTY_ERR);

        retval = -1;
    }
    else {

        /* Configure server socket as reusable */
        if (0 > setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrState, sizeof(reuseAddrState))) {

            perror("setsockopt");
            fflush(stderr);
            createLogMessage(STR_LOG_MSG_FUNC2_SOCK_CONF_FAIL, LOG_SVRTY_ERR);

            close(serverSocketFd);
            retval = -1;
        }
        else {

            /* Configure IPv6 address structure */
            memset(&serverAddress, 0, sizeof(serverAddress));
            serverAddress.sin6_family = AF_INET6;
            serverAddress.sin6_addr = in6addr_any;
            serverAddress.sin6_port = htons(NUM_SERVER_PORT);

            /* Bind server socket to server address */
            if (0 > bind(serverSocketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) {

                perror("bind");
                fflush(stderr);
                createLogMessage(STR_LOG_MSG_FUNC2_SOCK_BIND_FAIL, LOG_SVRTY_ERR);

                close(serverSocketFd);
                retval = -1;
            }
            else {

                /* Set server socket to passive (be able to accept incoming connections) */
                if (0 > listen(serverSocketFd, NUM_SERVER_PEND_QUEUE_LIMIT)) {

                    perror("listen");
                    fflush(stderr);
                    createLogMessage(STR_LOG_MSG_FUNC2_SOCK_LISTEN_FAIL, LOG_SVRTY_ERR);

                    close(serverSocketFd);
                    retval = -1;
                }
            }
        }
    }

    return retval;
}

static int startDroneServiceThreads(void) {

    int retval = 0;
    int i, *threadId = NULL;

    /* Create drone service threads */
    for (i = 0; i < NUM_DRONE_SRVC_THRD_POOL_SIZE; ++i) {

        threadId = (int*)malloc(sizeof(int));
        *threadId = i;
        if (pthread_create(&(droneServiceThreadPool[i]), NULL, threadFuncDroneService, threadId)) {

            /* Failed to create drone service thread */
            fprintf(stdout, STR_LOG_MSG_FUNC3_THRD_CREAT_FAIL, i);
            fflush(stdout);
            syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC3_THRD_CREAT_FAIL, i);

            retval = -1;
        }
    }

    return retval;
}

static void *threadFuncDroneService(void *arg) {

    int errorCode;
    int exitCondition;
    int keepAliveState = 1;
    int threadId = *((int *)arg);
    int serviceSocket = SOCK_FD_INVAL;
    char testBuff[1] = {0};
    char clientHostName[NI_MAXHOST] = {0};
    char clientServiceName[NI_MAXSERV] = {0};
    struct sockaddr_storage clientAddress;
    struct pollfd pollArray[NUM_POLL_ARR_SIZE];
    socklen_t clientAddressLength = sizeof(clientAddress);
    GstElement *pipeline = NULL;
    LoginMessageField_T droneID = 0U;
    // Use thread context wrapper if more params needed to be passed as arguments

    free(arg);

    while (1) {

        /* Try to lock server socket and wait for client connection */
        pthread_mutex_lock(&serverSocketLock);
        serviceSocket = accept(serverSocketFd, (struct sockaddr *)(&clientAddress), &clientAddressLength);
        pthread_mutex_unlock(&serverSocketLock);

        /* Check success of accepting new connection */
        if (serviceSocket < 0) {

            perror("accept");
            fflush(stderr);
            fprintf(stdout, STR_LOG_MSG_FUNC4_CONN_ACCEPT_FAIL, threadId);
            fflush(stdout);
            syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_CONN_ACCEPT_FAIL, threadId);
        }
        else {

            /* Try to resolve client name by address */
            errorCode = getnameinfo(
                
                (struct sockaddr*)(&clientAddress),
                clientAddressLength,
                clientHostName,
                sizeof(clientHostName),
                clientServiceName,
                sizeof(clientServiceName),
                0
            );
            if(0 == errorCode) {

                fprintf(stdout, STR_LOG_MSG_FUNC4_DRONE_ADDR_RES, threadId, clientHostName, clientServiceName);
                fflush(stdout);
                syslog(LOG_USER | LOG_INFO, STR_LOG_MSG_FUNC4_DRONE_ADDR_RES, threadId, clientHostName, clientServiceName);
            }
            else {

                fprintf(stdout, STR_LOG_MSG_FUNC4_DRONE_ADDR_RES_FAIL, threadId, gai_strerror(errorCode));
                fflush(stdout);
                syslog(LOG_USER | LOG_INFO, STR_LOG_MSG_FUNC4_DRONE_ADDR_RES_FAIL, threadId, gai_strerror(errorCode));
            }

            /* Set service socket option SO_KEEPALIVE for enhanced safety */
            if (0 > setsockopt(serviceSocket, SOL_SOCKET, SO_KEEPALIVE, &keepAliveState, sizeof(keepAliveState))) {

                perror("setsockopt");
                fflush(stderr);
                fprintf(stdout, STR_LOG_MSG_FUNC4_SOCK_CONF_FAIL, threadId);
                fflush(stdout);
                syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_SOCK_CONF_FAIL, threadId);
            }

            /* Authenticate drone */
            if (0 > authDrone(serviceSocket, &droneID)) {

                fprintf(stdout, STR_LOG_MSG_FUNC4_DRONE_AUTH_FAIL, threadId, droneID);
                fflush(stdout);
                syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_DRONE_AUTH_FAIL, threadId, droneID);

                close(serviceSocket);
                serviceSocket = SOCK_FD_INVAL;
            }
            else {

                /* Drone authentication succeded */
                fprintf(stdout, STR_LOG_MSG_FUNC4_DRONE_AUTH_SUCCESS, threadId, droneID);
                fflush(stdout);
                syslog(LOG_USER | LOG_INFO, STR_LOG_MSG_FUNC4_DRONE_AUTH_SUCCESS, threadId, droneID);

                /* Initialize poll array and exit condition */
                pollArray[IDX_POLL_ARR_CLI].events = POLLIN;
                pollArray[IDX_POLL_ARR_CLI].fd = STDIN_FILENO;
                pollArray[IDX_POLL_ARR_SOCK].events = POLLIN;
                pollArray[IDX_POLL_ARR_SOCK].fd = serviceSocket;

                exitCondition = 0;

                /* Communication loop */
                while (!exitCondition) {

                    if (poll(pollArray, NUM_POLL_ARR_SIZE, -1) > 0) {

                        if (pollArray[IDX_POLL_ARR_SOCK].revents & (POLLERR | POLLHUP)) {

                            /* Connection lost or closed by the drone */
                            fprintf(stdout, STR_LOG_MSG_FUNC4_CONN_CLOSED, threadId);
                            fflush(stdout);
                            syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_CONN_CLOSED, threadId);

                            exitCondition = 1;
                        }
                        else if (pollArray[IDX_POLL_ARR_SOCK].revents & (POLLIN)) {

                            /* There is available data and needs to be tested against EOF */

                            if(0 == recv(pollArray[IDX_POLL_ARR_SOCK].fd, testBuff, sizeof(testBuff), (MSG_DONTWAIT | MSG_PEEK))) {

                                /* Connection lost or closed by the drone */
                                fprintf(stdout, STR_LOG_MSG_FUNC4_CONN_CLOSED, threadId);
                                fflush(stdout);
                                syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_CONN_CLOSED, threadId);

                                exitCondition = 1;
                            }
                            else {

                                /* Handle incoming drone message */
                                if(inputMessageHandler(pollArray[IDX_POLL_ARR_SOCK].fd, &pipeline)) {
                                    syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_MSG_HANDLE_FAIL, threadId);
                                }
                                // TODO Update exit condition if needed
                            }
                        }

                        if ((pollArray[IDX_POLL_ARR_CLI].revents & (POLLIN)) && (!exitCondition)) {

                            /* Handle CLI user input */
                            if(inputCommandHandler(pollArray[IDX_POLL_ARR_SOCK].fd, &exitCondition, &pipeline)) {
                                syslog(LOG_USER | LOG_ERR, STR_LOG_MSG_FUNC4_CLI_HANDLE_FAIL, threadId);
                            }
                        }
                    }
                }

                /* Free pipeline */
                if(NULL != pipeline) {
                    gst_element_set_state(pipeline, GST_STATE_NULL);
                    gst_object_unref(pipeline);
                    pipeline = NULL;
                }

                // Stop auxiliary threads if necessary

                /* Close service socket */
                close(serviceSocket);
                serviceSocket = SOCK_FD_INVAL;
            }
        }
    }

    return NULL;
}

static int authDrone(const int serviceSocket, LoginMessageField_T *droneID) {

    int retval = 0;
    int length;
    LoginMessageField_T loginMessage[NUM_LOGIN_MSG_SIZE] = {0};

    if ((0 > serviceSocket) || (NULL == droneID)) {

        createLogMessage(STR_LOG_MSG_FUNC5_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }
    else {

        /* Receive login message */
        length = recvTimeout(serviceSocket, loginMessage, sizeof(loginMessage), MSG_WAITALL, 2, 0);
        if (sizeof(loginMessage) > length) {

            if (0 > length) {

                perror("recv");
                fflush(stderr);
            }
            createLogMessage(STR_LOG_MSG_FUNC5_LOGIN_RECV_FAIL, LOG_SVRTY_ERR);

            retval = -1;
        }
        else {

            if ((LoginMessageField_T)MOD_MSG_CODE_LOGIN == loginMessage[IDX_LOGIN_MSG_CODE]) {

                *droneID = loginMessage[IDX_LOGIN_MSG_ID];
                // Validate drone ID here if needed 

                loginMessage[IDX_LOGIN_MSG_CODE] = (LoginMessageField_T)MOD_MSG_CODE_LOGIN_ACK;
                length = send(serviceSocket, loginMessage, sizeof(loginMessage), MSG_NOSIGNAL);
                if (0 > length) {

                    perror("send");
                    fflush(stderr);
                    createLogMessage(STR_LOG_MSG_FUNC5_LOGIN_SEND_FAIL, LOG_SVRTY_ERR);

                    retval = -1;
                }
            }
            else {

                loginMessage[IDX_LOGIN_MSG_CODE] = (LoginMessageField_T)MOD_MSG_CODE_LOGIN_NACK;
                loginMessage[IDX_LOGIN_MSG_ID] = 0U;
                length = send(serviceSocket, loginMessage, sizeof(loginMessage), MSG_NOSIGNAL);
                if (0 > length) {

                    perror("send");
                    fflush(stderr);
                    createLogMessage(STR_LOG_MSG_FUNC5_LOGIN_SEND_FAIL, LOG_SVRTY_ERR);

                    retval = -1;
                }
            }
        }
    }

    return retval;
}

static int inputMessageHandler(const int serviceSocket, GstElement* *pipeline) {

    int retval = 0;
    int length;
    MessageHeaderField_T messageHeader[NUM_MSG_HEADER_SIZE] = {0};

    if ((0 > serverSocketFd) || (NULL == pipeline)) {

        createLogMessage(STR_LOG_MSG_FUNC8_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }
    else {

        length = recvTimeout(serviceSocket, messageHeader, sizeof(messageHeader), MSG_WAITALL, 2, 0);
        if (length < 0) {

            perror("recv");
            createLogMessage(STR_LOG_MSG_FUNC8_MSG_RECV_FAIL, LOG_SVRTY_ERR);

            retval = -1;
        }
        else {

            /*
             * Since there is only one central module (GC Common) no
             * need to check module message address.
             */

            switch((ModuleMessageCode_T)(messageHeader[IDX_MSG_HEADER_CODE])) {

                case MOD_MSG_CODE_STREAM_ERROR:

                    printf("\n[WARNING]: Video stream closed due to internal error on drone side.\n");
                    fflush(stdout);
                    if(stopStream(pipeline)) {

                        createLogMessage(STR_LOG_MSG_FUNC8_STRM_STOP_FAIL, LOG_SVRTY_ERR);
                        retval = -1;
                    }
                    break;

                default:

                    /* Invalid module message received. Clean up RX buffer. */
                    cleanupInputMessages(serverSocketFd);
                    createLogMessage(STR_LOG_MSG_FUNC8_MSG_RECV_INVAL, LOG_SVRTY_WRN);
                    break;
            }
        }
    }

    return retval;
}

static int inputCommandHandler(const int serviceSocket, int *exitCondition, GstElement* *pipeline) {

    int retval = 0;
    int cmdArgIndex = 0;
    const char* cmdArgs[NUM_MAX_CMD_ARGS] = {0};
    const char delim[] = " ";
    char cmdInputBuffer[NUM_CMD_BUFF_SIZE] = {0};

    if ((0 > serviceSocket) || (NULL == pipeline) || (NULL == exitCondition)) {

        createLogMessage(STR_LOG_MSG_FUNC9_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }
    else {

        /*
         * Read user command and remove the new line character in
         * order to support single word commands.
         */
        read(STDIN_FILENO, cmdInputBuffer, (sizeof(cmdInputBuffer) - 1));
        cmdInputBuffer[strlen(cmdInputBuffer)-1] = '\0';

        /* Parse user command */
        cmdArgs[cmdArgIndex] = strtok(cmdInputBuffer, delim);
        while((NULL != cmdArgs[cmdArgIndex]) && ((NUM_MAX_CMD_ARGS-1) > cmdArgIndex)) {
     
            cmdArgIndex++;
            cmdArgs[cmdArgIndex] = strtok(NULL, delim);
        }

        /* Interpret user command */
        if(NULL != cmdArgs[0]) {

            if(0 == strcmp(cmdArgs[0], STR_USR_CMD_STRM_PLAY)) {

                /* Request video stream */
                printf(">> Ground control requested video stream <<\n");
                fflush(stdout);
                if(requestStream(serviceSocket, pipeline)) {
                    createLogMessage(STR_LOG_MSG_FUNC9_REQ_STRM_FAIL, LOG_SVRTY_ERR);
                    retval = -1;
                }
            }    
            else if(0 == strcmp(cmdArgs[0], STR_USR_CMD_STRM_STOP)) {

                /* Stop video stream */
                printf(">> Ground control stopped video stream <<\n");
                fflush(stdout);
                if(stopStream(pipeline)) {
                    createLogMessage(STR_LOG_MSG_FUNC9_STOP_STRM_FAIL, LOG_SVRTY_ERR);
                    retval = -1;
                }
                if(sendStopMessage(serviceSocket)) {
                    createLogMessage(STR_LOG_MSG_FUNC9_STOP_STRM_FAIL, LOG_SVRTY_ERR);
                    retval = -1;
                }
            }
            else if(0 == strcmp(cmdArgs[0], STR_USR_CMD_DRN_DCON)) {

                /* Disconnect drone */
                printf(">> Ground control disconnected drone <<\n");
                fflush(stdout);
                *exitCondition = 1;
            }
            else {

                /* Invalid user command */
                printf("\nInvalid command. Possible commands are:\n\n\tplay - Request video stream\n\tstop - Stop video stream\n\tdconn - Disconnect drone\n\n");
                fflush(stdout);
                retval = -1;
            }
        }
    }

    return retval;
}

static int sendStopMessage(const int serviceSocket) {

    int retval = 0;
    int length;
    MessageHeaderField_T messageHeader[NUM_MSG_HEADER_SIZE] = {0};

    if (0 > serviceSocket) {

        createLogMessage(STR_LOG_MSG_FUNC11_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }
    else {

        messageHeader[IDX_MSG_HEADER_MODULE] = MOD_NAME_STREAM;
        messageHeader[IDX_MSG_HEADER_CODE] = MOD_MSG_CODE_STREAM_STOP;
        length = send(serviceSocket, messageHeader, sizeof(messageHeader), MSG_NOSIGNAL);
        if (0 > length) {

            perror("send");
            createLogMessage(STR_LOG_MSG_FUNC11_MSG_SEND_FAIL, LOG_SVRTY_ERR);
            retval = -1;
        }
    }

    return retval;
}

static void cleanupInputMessages(const int sockFd) {

    char data[256];
    while (0 < recv(sockFd, data, sizeof(data), MSG_NOSIGNAL | MSG_DONTWAIT)) {

        // NOP
    }
}
