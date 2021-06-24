/**
 * @file        com_utils.c
 * @author      Adam Csizy
 * @date        2021-04-01
 * @version     v1.1.0
 * 
 * @brief       Communication utilities and network module
 */


#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "com_utils.h"
#include "log_utils.h"
#include "stream_utils.h"


/* Communication related macro definitions */

#define MessageHeaderField_T        uint32_t    /**< Type of the fields in the header of network messages */
#define LoginMessageField_T         uint32_t    /**< Type of the fields in the login netork message */

#define STR_FORMAT_MOD_MSG          "\nModule Message:\n\tAddress: %d\n\tCode: %d\n"    /**< Module message string format */
#define SOCK_FD_INVAL               -1  /**< Invalid socket file descriptor */
#define RECONNECT_COOLDOWN_SEC      10U /**< Cooldown in seconds before initiating a reconnection to the ground control */
#define NUM_GC_ADDR_SIZE            64U /**< Size of ground control address string */
#define NUM_GC_PORT_SIZE            16U /**< Size of ground control port string */
#define STR_GC_ADDR_DEFAULT_LAN     "195.441.0.134" /**< Default address of ground control (LAN) */
#define STR_GC_ADDR_DEFAULT_WAN     "any_custom_domain.ddns.net" /**< Default address of ground control (WAN) */
#define STR_GC_PORT_DEFAULT_LAN     "5010" /**< Default port of ground control (LAN) */
#define STR_GC_PORT_DEFAULT_WAN     "17010" /**< Default port of ground control (WAN) */
#define RECV_LOGIN_MSG_COOLDOWN_SEC 4U  /**< Cooldown in seconds between attempts to receive login message from ground control */
#define NUM_POLL_ARRAY_SIZE         1U  /**< Size of poll array */
#define IDX_SOCK                    0U  /**< Socket index */
#define NUM_NETWORK_MSGQ_SIZE       16U /**< Size of network module's message queue */
#define NUM_MSG_HEADER_SIZE         2U  /**< Size of message header array in MessageHeaderField_T */
#define IDX_MSG_HEADER_MODULE       0U  /**< Index of module name in message header array */
#define IDX_MSG_HEADER_CODE         1U  /**< Index of module message code in message header array */
#define NUM_LOGIN_MSG_SIZE          2U  /**< Size of login message array in LoginMessageField_T */
#define IDX_LOGIN_MSG_CODE          0U  /**< Index of module message code in login message array */
#define IDX_LOGIN_MSG_ID            1U  /**< Index of drone ID in login message array */

/* Communication related global variable declarations */

ModuleMessageQueue_T networkMsgq;


/* Communication related static variable declarations */

static pthread_mutex_t socketFdLock = PTHREAD_MUTEX_INITIALIZER;    /**< Mutex to protect network socket */
static pthread_t threadNetworkOut;      /**< Thread object for handling network output */
static pthread_t threadNetworkIn;       /**< Thread object for handling network input */


/* Communication related static function declarations */

/**
 * @brief       Estabilish connection with ground control.
 * 
 * @details     Estabilishes TCP connection with a ground control
 *              node specified by the input arguments (IP, port).
 *              In case of successful connection the given socket
 *              file descriptor is set and can be used for further
 *              communication.
 * 
 * @note        The companion computer sends a LOGIN message code
 *              and the drone's ID. In turn it receives a LOGIN_ACK
 *              message code and the drone's ID.
 *
 * @param[in]   node Network address (IP) of ground control node.
 * @param[in]   size Service name (port) of ground control node.
 * @param[out]  fd Socket file descriptor associated with the ground control connection.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int connectToGroundControl(const char *node, const char *service, int *fd);

/**
 * @brief       Handle input message.
 * 
 * @details     Handles input messages received over IP/TCP
 *              from the ground control. Message handling
 *              includes parsing message code and target
 *              module name specified in the message header
 *              as well as calling the corresponding message
 *              handlers per module. On failure the network
 *              RX buffer is cleaned up to preserve consistency. 
 *
 * @note        Not thread safe. In case of reading the network
 *              socket from multiple threads simultaneously
 *              mutual exclusion must be applied using static
 *              variable 'socketFDLock'.
 * 
 * @param[in]   sockFd Network socket file descriptor.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int inputMessageHandler(const int sockFd);

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
 *              mutual exclusion must be applied using static
 *              variable 'socketFDLock'.
 * 
 * @param[in]   sockFd Network socket file descriptor.
 */
static void cleanupInputMessages(const int sockFd);

/**
 * @brief       Convert network data to stream module message.
 * 
 * @details     Converts network data to stream module message.
 *              This function (optionally) reads data from the
 *              network and creates a module message object based
 *              on the given module message code and network data.
 *              On successful message object creation the object
 *              is inserted into the stream module's message queue.
 *
 * @note        Not thread safe. In case of reading the network
 *              socket from multiple threads simultaneously
 *              mutual exclusion must be applied using static
 *              variable 'socketFDLock'.
 * 
 * @param[in]   sockFd Network socket file descriptor.
 * @param[in]   code Stream module message code.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int networkToStreamMessage(const int sockFd, const ModuleMessageCode_T code);

/**
 * @brief       Convert GC common module message to network data.
 * 
 * @details     Converts GC (Ground Control) common module message
 *              to network data. This function parses the given
 *              modul message object and creates a byte stream
 *              which is then sent to the ground control over
 *              IP/TCP using the given network socket descriptor.
 *
 * @note        Thread safe.
 * 
 * @param[in]   sockFd Network socket file descriptor.
 * @param[in]   message Message object to be parsed.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int gccommonMessageToNetwork(const int *sockFd, const ModuleMessage_T *message);

/**
 * @brief       Start routine of network input handler thread.
 * 
 * @details     Estabilishes connection with ground control and
 *              handles incoming network traffic over IP/TCP.
 *              Incoming messages are parsed and forwarded to the
 *              corresponding module's message queue.
 * 
 * @note        Critical thread. On failure connection with the
 *              ground control is lost.
 * 
 * @param[in]   arg Network module initialization context.
 * 
 * @return      Any (not used).
 */
static void* threadFuncNetworkIn(void *arg);

/**
 * @brief       Start routine of network output handler thread.
 * 
 * @details     Handles outgoing network traffic over IP/TCP.
 *              Messages from the network module's message queue
 *              are parsed and forwarded to the ground control.
 * 
 * @note        Critical thread. On failure connection with the
 *              ground control is lost.
 * 
 * @param[in]   arg Launch argument (not used).
 * 
 * @return      Any (not used).
 */
static void* threadFuncNetworkOut(void *arg);


/* Communication related function definitions */

int printModuleMessage(const ModuleMessage_T *message) {

    int retval = 0;

    if(NULL != message) {

        fflush(stdout);
        fprintf(
            
            stdout,
            STR_FORMAT_MOD_MSG,
            message->address,
            message->code
        );
        fflush(stdout);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC11_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

int initModuleMessageQueue(ModuleMessageQueue_T *messageQueue, const size_t size) {

    int retval = 0;
    pthread_mutexattr_t mutexAttribute;
    pthread_condattr_t condvarAttribute;

    if((NULL != messageQueue) && (0 != size) && (0 == (size & (size-1)))) {

        messageQueue->messages = (ModuleMessage_T**)calloc(size, sizeof(ModuleMessage_T*));
        if(NULL != messageQueue->messages) {

            messageQueue->size = size;
            messageQueue->front = 0;
            messageQueue->back = 0;

            if(pthread_mutexattr_init(&mutexAttribute)) {
                createLogMessage(STR_LOG_MSG_FUNC7_MTX_ATTR_INIT_FAIL, LOG_SVRTY_ERR);
                retval = -1;
            }
            else if(pthread_mutexattr_settype(&mutexAttribute, PTHREAD_MUTEX_DEFAULT)) {
                createLogMessage(STR_LOG_MSG_FUNC7_MTX_ATTR_SET_TYPE_FAIL, LOG_SVRTY_ERR);
                retval = -1;
            }
            else if(pthread_mutex_init(&(messageQueue->lock), &mutexAttribute)) {
                createLogMessage(STR_LOG_MSG_FUNC7_MTX_INIT_FAIL, LOG_SVRTY_ERR);
                retval = -1;
            }
            else if(pthread_condattr_init(&condvarAttribute)) {
                createLogMessage(STR_LOG_MSG_FUNC7_COND_ATTR_INIT_FAIL, LOG_SVRTY_ERR);
                retval = -1;
            }
            else if(pthread_cond_init(&(messageQueue->update), &condvarAttribute)) {
                createLogMessage(STR_LOG_MSG_FUNC7_COND_INIT_FAIL, LOG_SVRTY_ERR);
                retval = -1;
            }
        }
        else {

            createLogMessage(STR_LOG_MSG_FUNC7_MEM_ALLOC_FAIL, LOG_SVRTY_ERR);
            retval = -1;
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC7_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

int deinitModuleMessageQueue(ModuleMessageQueue_T *messageQueue) {

    int retval = 0;

    if(NULL != messageQueue) {

        while(NULL != messageQueue->messages[messageQueue->back]) {

            free(messageQueue->messages[messageQueue->back]);
            messageQueue->messages[messageQueue->back] = NULL;
            messageQueue->back = ((messageQueue->back + 1) & (messageQueue->size - 1));
        }

        free(messageQueue->messages);
        messageQueue->messages = NULL;
        messageQueue->size = 0U;

        if(pthread_mutex_destroy(&(messageQueue->lock))) {
            createLogMessage(STR_LOG_MSG_FUNC8_MTX_DSTRY_FAIL, LOG_SVRTY_ERR);
            retval = -1;
        }
        if(pthread_cond_destroy(&(messageQueue->update))) {
            createLogMessage(STR_LOG_MSG_FUNC8_COND_DSTRY_FAIL, LOG_SVRTY_ERR);
            retval = -1;
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC8_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

int insertModuleMessage(ModuleMessageQueue_T *messageQueue, ModuleMessage_T *message, const int noblock) {

    int retval = 0;

    if((NULL != messageQueue) && (NULL != message)) {

        if(noblock) {

            if(pthread_mutex_trylock(&(messageQueue->lock))) {

                retval = -2;
                return retval;
            }
            else if(NULL != messageQueue->messages[messageQueue->front]) {

                pthread_mutex_unlock(&(messageQueue->lock));
                retval = -3;
                return retval;
            }
        }
        else {

            pthread_mutex_lock(&(messageQueue->lock));
            while(NULL != messageQueue->messages[messageQueue->front]) {

                pthread_cond_wait(&(messageQueue->update), &(messageQueue->lock));
            }
        }
        messageQueue->messages[messageQueue->front] = message;
        messageQueue->front = ((messageQueue->front + 1) & (messageQueue->size - 1));
        pthread_cond_broadcast(&(messageQueue->update));
        pthread_mutex_unlock(&(messageQueue->lock));
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC9_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

int removeModuleMessage(ModuleMessageQueue_T *messageQueue, ModuleMessage_T* *message, const int noblock) {

    int retval = 0;

    if((NULL != messageQueue) && (NULL != message)) {

        if(noblock) {

            if(pthread_mutex_trylock(&(messageQueue->lock))) {

                retval = -2;
                return retval;
            }
            else if(NULL == messageQueue->messages[messageQueue->back]) {

                pthread_mutex_unlock(&(messageQueue->lock));
                retval = -3;
                return retval;
            }
        }
        else {

            pthread_mutex_lock(&(messageQueue->lock));
            while(NULL == messageQueue->messages[messageQueue->back]) {

                pthread_cond_wait(&(messageQueue->update), &(messageQueue->lock));
            }
        }
        *message = messageQueue->messages[messageQueue->back];
        messageQueue->messages[messageQueue->back] = NULL;
        messageQueue->back = ((messageQueue->back + 1) & (messageQueue->size - 1));
        pthread_cond_broadcast(&(messageQueue->update));
        pthread_mutex_unlock(&(messageQueue->lock));
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC10_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

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

int initNetworkModule(NetworkInitContext_T *initCtx) {

    int retval = 0;

    if(initModuleMessageQueue(&networkMsgq, NUM_NETWORK_MSGQ_SIZE)) {

        createLogMessage(STR_LOG_MSG_FUNC19_MSGQ_INIT_FAIL, LOG_SVRTY_ERR);

        retval = -1;
        return retval;
    }

    if(pthread_create(&threadNetworkIn, NULL, threadFuncNetworkIn, (void*)initCtx)) {

        createLogMessage(STR_LOG_MSG_FUNC19_THRD_IN_START_FAIL, LOG_SVRTY_ERR);

        deinitModuleMessageQueue(&networkMsgq);
        retval = -1;
        return retval;
    }

    return retval;
}

static void* threadFuncNetworkOut(void *arg) {

    int *socketFileDescriptor = (int*)arg;
    ModuleMessage_T *message = NULL;

    while(1) {

        if(0 != removeModuleMessage(&networkMsgq, &message, MOD_MSGQ_BLOCK)) {

            createLogMessage(STR_LOG_MSG_FUNC14_MSG_RMV_FAIL, LOG_SVRTY_ERR);
        }
        else {

            switch(message->address) {

                case MOD_NAME_GCCOMMON:

                    /* Ground control common module */
                    if(gccommonMessageToNetwork(socketFileDescriptor, message)) {

                        createLogMessage(STR_LOG_MSG_FUNC17_PROC_MSG_CMN_FAIL, LOG_SVRTY_WRN);
                    }
                    break;

                default:

                    /* Unknown module */
                    createLogMessage(STR_LOG_MSG_FUNC17_MOD_NAME_INVAL, LOG_SVRTY_WRN);
                    break;
            }

            free(message);
            message = NULL;

            // Simultanous socket read-write does not require mutex
            // if theres only 1 read and 1 write thread, but in
            // case of reconnecting mutex is needed to not to mess up
            // the connection mechanism
        }
    }

    return NULL;
}

static void* threadFuncNetworkIn(void *arg) {

    int errorCode = 0;
    char testBuf[1] = {0};
    char gcAddress[NUM_GC_ADDR_SIZE] = STR_GC_ADDR_DEFAULT_WAN;
    char gcPort[NUM_GC_PORT_SIZE] = STR_GC_PORT_DEFAULT_WAN;
    NetworkInitContext_T *initCtx = (NetworkInitContext_T*)arg;
    struct pollfd pollArray[NUM_POLL_ARRAY_SIZE] = {0};

    /* Initialize network context */
    if(initCtx) {

        if(initCtx->serverNodeName) {
            memset(gcAddress, 0, sizeof(gcAddress));
            strncpy(gcAddress, initCtx->serverNodeName, sizeof(gcAddress)-1);
        }
        if(initCtx->serverServiceName) {
            memset(gcPort, 0, sizeof(gcPort));
            strncpy(gcPort, initCtx->serverServiceName, sizeof(gcPort)-1);
        }
        free(initCtx);
    }

    /* Connect to ground control */
    while(connectToGroundControl(gcAddress, gcPort, &(pollArray[IDX_SOCK].fd))) {

        #ifdef CC_DEBUG_MODE
        fprintf(stdout, STR_LOG_MSG_FUNC13_CONN_FAIL_RETRY, RECONNECT_COOLDOWN_SEC);
        #endif
        syslog(LOG_DAEMON | LOG_WARNING, STR_LOG_MSG_FUNC13_CONN_FAIL_RETRY, RECONNECT_COOLDOWN_SEC);

        sleep(RECONNECT_COOLDOWN_SEC);
    }

    /* Start network output handler thread */
    errorCode = pthread_create(&threadNetworkOut, NULL, threadFuncNetworkOut, &(pollArray[IDX_SOCK].fd));
    if(0 != errorCode) {

        createLogMessage(STR_LOG_MSG_FUNC13_THRD_START_FAIL, LOG_SVRTY_ERR);
        kill(getpid(), SIGTERM);
        pthread_exit(NULL);
    }

    pollArray[IDX_SOCK].events = POLLIN;

    /* Launch own thread loop */
    while(1) {

        if(0 < poll(pollArray, NUM_POLL_ARRAY_SIZE, -1)) {

            if(pollArray[IDX_SOCK].revents & (POLLERR | POLLHUP)) {

                /* Connection lost/closed to ground control */
                createLogMessage(STR_LOG_MSG_FUNC13_GC_CONN_CLOSED, LOG_SVRTY_WRN);

                pthread_mutex_lock(&socketFdLock);
                close(pollArray[IDX_SOCK].fd);
                pollArray[IDX_SOCK].fd = SOCK_FD_INVAL;
                
                /* Reconnect to ground control */
                while(connectToGroundControl(gcAddress, gcPort, &(pollArray[IDX_SOCK].fd))) {

                    #ifdef CC_DEBUG_MODE
                    fprintf(stdout, STR_LOG_MSG_FUNC13_RECONN_FAIL_RETRY, RECONNECT_COOLDOWN_SEC);
                    #endif
                    syslog(LOG_DAEMON | LOG_WARNING, STR_LOG_MSG_FUNC13_RECONN_FAIL_RETRY, RECONNECT_COOLDOWN_SEC);

                    sleep(RECONNECT_COOLDOWN_SEC);
                }

                pthread_mutex_unlock(&socketFdLock);
            }
            else if(pollArray[IDX_SOCK].revents & (POLLIN)) {

                if(0 == recv(pollArray[IDX_SOCK].fd, testBuf, sizeof(testBuf), (MSG_DONTWAIT | MSG_PEEK))) {

                    /* Connection lost/closed to ground control */
                    createLogMessage(STR_LOG_MSG_FUNC13_GC_CONN_CLOSED, LOG_SVRTY_WRN);

                    pthread_mutex_lock(&socketFdLock);
                    close(pollArray[IDX_SOCK].fd);
                    pollArray[IDX_SOCK].fd = SOCK_FD_INVAL;
                    
                    /* Reconnect to ground control */
                    while(connectToGroundControl(gcAddress, gcPort, &(pollArray[IDX_SOCK].fd))) {

                        #ifdef CC_DEBUG_MODE
                        fprintf(stdout, STR_LOG_MSG_FUNC13_RECONN_FAIL_RETRY, RECONNECT_COOLDOWN_SEC);
                        #endif
                        syslog(LOG_DAEMON | LOG_WARNING, STR_LOG_MSG_FUNC13_RECONN_FAIL_RETRY, RECONNECT_COOLDOWN_SEC);

                        sleep(RECONNECT_COOLDOWN_SEC);
                    }

                    pthread_mutex_unlock(&socketFdLock);
                }
                else {
                    
                    /* Data available */
                    inputMessageHandler(pollArray[IDX_SOCK].fd);
                }
            }
        }
    }

    return NULL;
}

static int connectToGroundControl(const char *node, const char *service, int *fd) {

    int retval = 0;
    int errorCode = 0;
    LoginMessageField_T loginMessage[NUM_LOGIN_MSG_SIZE] = {0};
    int length;
    int keepAliveState = 1;
    
    struct addrinfo hints;
    struct addrinfo *result;
    
    /* Check if function arguments are valid */
    if((NULL != fd) && (NULL != node) && (NULL != service)) {

        /* Initialize hints structure with TCP and either IPv4 or IPv6 address */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        /* Resolve ground control address */
        errorCode = getaddrinfo(node, service, &hints, &result);
        if(errorCode) {
        
            /* Failed to resolve ground control address */
            #ifdef CC_DEBUG_MODE
            fprintf(stdout, STR_LOG_MSG_FUNC12_GETADDRINFO_FAIL, gai_strerror(errorCode));
            fflush(stdout);
            #endif
            syslog(LOG_DAEMON | LOG_ERR, STR_LOG_MSG_FUNC12_GETADDRINFO_FAIL, gai_strerror(errorCode));
            createLogMessage(STR_LOG_MSG_FUNC12_GC_ADDR_RESLV_FAIL, LOG_SVRTY_ERR);
        
            *fd = SOCK_FD_INVAL;
            retval = -1;
            return retval;
        }
        else if(NULL == result) {
        
            /* No ground control found with the given parameters */
            createLogMessage(STR_LOG_MSG_FUNC12_GC_NOT_FOUND, LOG_SVRTY_ERR);

            *fd = SOCK_FD_INVAL;
            retval = -1;
            return retval;
        }

        /* Create socket based on the result's settings */
        *fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(*fd < 0) {
        
            /* Failed to create socket */
            #ifdef CC_DEBUG_MODE
            perror("socket");
            fflush(stderr);
            #endif
            createLogMessage(STR_LOG_MSG_FUNC12_CREAT_SOCK_FAIL, LOG_SVRTY_ERR);
        
            /* Free resources */
            freeaddrinfo(result);
            *fd = SOCK_FD_INVAL;

            retval = -1;
            return retval;
        }

        /* Connect socket to ground control referenced to by ai_addr */
        if(connect(*fd, result->ai_addr, result->ai_addrlen) < 0) {
        
            /* Failed to connect to ground control */
            #ifdef CC_DEBUG_MODE
            perror("connect");
            fflush(stderr);
            #endif
            createLogMessage(STR_LOG_MSG_FUNC12_GC_CONN_FAIL, LOG_SVRTY_ERR);
        
            /* Free resources */
            freeaddrinfo(result);
            *fd = SOCK_FD_INVAL;

            retval = -1;
            return retval;
        }

        /* Send login message to ground control */
        loginMessage[IDX_LOGIN_MSG_CODE] = (LoginMessageField_T)MOD_MSG_CODE_LOGIN;
        loginMessage[IDX_LOGIN_MSG_ID] = DRONE_ID;
        length = send(*fd, loginMessage, sizeof(loginMessage), MSG_NOSIGNAL);
        if((length < 0) || (length < sizeof(loginMessage))) {
        
            /* Failed to send login message to ground control */
            #ifdef CC_DEBUG_MODE
            perror("send");
            fflush(stderr);
            #endif
            createLogMessage(STR_LOG_MSG_FUNC12_LOGIN_SEND_FAIL, LOG_SVRTY_ERR);
        
            /* Free resources */
            freeaddrinfo(result);
            close(*fd);
            *fd = SOCK_FD_INVAL;

            retval = -1;
            return retval;
        }

        /* Receive login message from ground control */
        memset(loginMessage, 0, sizeof(loginMessage));
        length = recvTimeout(*fd, loginMessage, sizeof(loginMessage), MSG_WAITALL, 2, 0);
        if(length < sizeof(loginMessage)) {
    
            /* Failed to receive login message from ground control */
            #ifdef CC_DEBUG_MODE
            perror("recv");
            fflush(stderr);
            #endif
            createLogMessage(STR_LOG_MSG_FUNC12_LOGIN_RECV_FAIL, LOG_SVRTY_ERR);

            /* Free resources */
            freeaddrinfo(result);
            close(*fd);
            *fd = SOCK_FD_INVAL;

            retval = -1;
            return retval;
        }
    
        /* Validate login message */
        if((loginMessage[IDX_LOGIN_MSG_CODE] != (LoginMessageField_T)MOD_MSG_CODE_LOGIN_ACK) || (loginMessage[IDX_LOGIN_MSG_ID] != DRONE_ID)) {

            /* Invalid login message acknowledgement from ground control */
            createLogMessage(STR_LOG_MSG_FUNC12_LOGIN_ACK_INVAL , LOG_SVRTY_ERR);

            /* Free resources */
            freeaddrinfo(result);
            close(*fd);
            *fd = SOCK_FD_INVAL;

            retval = -1;
            return retval;
        }

        /* Set socket option SO_KEEPALIVE for enhanced safety */
        if(setsockopt(*fd, SOL_SOCKET, SO_KEEPALIVE, &keepAliveState, sizeof(keepAliveState)) < 0) {
        
            /* Failed to set socket option SO_KEEPALIVE */
            #ifdef CC_DEBUG_MODE
            perror("setsockopt");
            fflush(stderr);
            #endif
            createLogMessage(STR_LOG_MSG_FUNC12_SET_KEEPALIVE_FAIL, LOG_SVRTY_WRN);
        }

        /* Free address-info list pointed by result */
        freeaddrinfo(result);
    
        /* Connection was successfully estabilished */
        #ifdef CC_DEBUG_MODE
        fprintf(stdout, STR_LOG_MSG_FUNC12_GC_CONN_SUCCESS, node, service);
        fflush(stdout);
        #endif
        syslog(LOG_DAEMON | LOG_INFO, STR_LOG_MSG_FUNC12_GC_CONN_SUCCESS, node, service);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC12_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }
    
    return retval;
}

static int inputMessageHandler(const int sockFd) {

    int retval = 0;
    int length;
    MessageHeaderField_T messageHeader[NUM_MSG_HEADER_SIZE] = {0};

    if(0 <= sockFd) {

        /* Read message header (module address and message code) */
        length = recvTimeout(sockFd, messageHeader, sizeof(messageHeader), MSG_WAITALL, 2, 0);
        if(length < 0) {

            /* Failed to receive message header */
            createLogMessage(STR_LOG_MSG_FUNC15_HDR_RECV_FAIL, LOG_SVRTY_ERR);
            cleanupInputMessages(sockFd);
            retval = -1;
        }
        else {

            /* Parse module name */
            switch((ModuleName_T)messageHeader[IDX_MSG_HEADER_MODULE]) {

                case MOD_NAME_STREAM:

                    if(networkToStreamMessage(sockFd, (ModuleMessageCode_T)messageHeader[IDX_MSG_HEADER_CODE])) {

                        createLogMessage(STR_LOG_MSG_FUNC15_PROC_MSG_STRM_FAIL, LOG_SVRTY_WRN);
                        cleanupInputMessages(sockFd);
                        retval = -1;
                    }
                    break;

                default:

                    createLogMessage(STR_LOG_MSG_FUNC15_MOD_NAME_INVAL, LOG_SVRTY_WRN);
                    cleanupInputMessages(sockFd);
                    retval = -1;
                    break;
            }
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC15_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
        return retval;
    }

    return retval;
}

static void cleanupInputMessages(const int sockFd) {

    char data[256];
    while(0 < recv(sockFd, data, sizeof(data), MSG_NOSIGNAL | MSG_DONTWAIT)) {

        // NOP
    }
}

static int networkToStreamMessage(const int sockFd, const ModuleMessageCode_T code) {

    int retval = 0;
    int length;
    ModuleMessage_T *message = NULL;

    if(0 <= sockFd) {
        
        message = (ModuleMessage_T*)calloc(1, sizeof(ModuleMessage_T));
        if(NULL != message) {

            message->address = MOD_NAME_STREAM;
            message->code = code;

            switch(code) {

                case MOD_MSG_CODE_STREAM_REQ:

                    /* Request video stream */
                    length = recvTimeout(sockFd, &(message->data.videoStreamPort),
                        sizeof(message->data.videoStreamPort), MSG_WAITALL, 2, 0);

                    /* 
                     * We can read directly into message data as long as
                     * the video stream port's type is fixed among platforms.
                     * (unsigned 32 bit integer currently)
                     */

                    if(sizeof(message->data.videoStreamPort) > length) {

                        if(0 > length) {
                            #ifdef CC_DEBUG_MODE
                            perror("send");
                            fflush(stderr);
                            #endif
                        }
                        createLogMessage(STR_LOG_MSG_FUNC16_STRM_PORT_RECV_FAIL, LOG_SVRTY_ERR);

                        free(message);
                        message = NULL;
                        retval = -1;
                        return retval;
                    }
                    break;

                case MOD_MSG_CODE_STREAM_START:

                    /* Start video stream */
                    // NOP
                    // On failure free message and return immediately
                    break;

                case MOD_MSG_CODE_STREAM_STOP:

                    /* Stop video stream */
                    // NOP
                    // On failure free message and return immediately
                    break;

                default:

                    /* Invalid module message code */
                    createLogMessage(STR_LOG_MSG_FUNC16_CODE_INVAL, LOG_SVRTY_WRN);

                    free(message);
                    message = NULL;
                    retval = -1;
                    return retval;
            }

            insertModuleMessage(&streamMsgq, message, MOD_MSGQ_BLOCK);
            message = NULL;
        }
        else {

            createLogMessage(STR_LOG_MSG_FUNC16_MSG_ALLOC_FAIL, LOG_SVRTY_ERR);
            retval = -1;
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC16_ARG_INVAL , LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

static int gccommonMessageToNetwork(const int *sockFd, const ModuleMessage_T *message) {

    int retval = 0;
    int length;
    MessageHeaderField_T messageHeader[NUM_MSG_HEADER_SIZE] = {0};
    uint32_t codingFormat = 0U;

    if((NULL != sockFd) && (NULL != message)) {

        messageHeader[IDX_MSG_HEADER_MODULE] = (MessageHeaderField_T)message->address;
        messageHeader[IDX_MSG_HEADER_CODE] = (MessageHeaderField_T)message->code;

        /* Send message header to network */
        pthread_mutex_lock(&socketFdLock);
        length = send(*sockFd, messageHeader, sizeof(messageHeader), MSG_NOSIGNAL);

        if(sizeof(messageHeader) > length) {

            if(0 > length) {
                #ifdef CC_DEBUG_MODE
                perror("send");
                fflush(stderr);
                #endif
            }
            createLogMessage(STR_LOG_MSG_FUNC18_HDR_SEND_FAIL, LOG_SVRTY_ERR);
            retval = -1;
        }
        else {

            /* Process message data if needed */
            switch(message->code) {

                case MOD_MSG_CODE_STREAM_TYPE:

                    codingFormat = (uint32_t)message->data.codingFormat;

                    /* Send video coding format to network */
                    length = send(*sockFd, &codingFormat, sizeof(codingFormat), MSG_NOSIGNAL);
                    if(sizeof(codingFormat) > length) {

                        if(0 > length) {
                            #ifdef CC_DEBUG_MODE
                            perror("send");
                            fflush(stderr);
                            #endif
                        }
                        createLogMessage(STR_LOG_MSG_FUNC18_DATA_SEND_FAIL, LOG_SVRTY_ERR);
                        retval = -1;
                    }
                    break;

                case MOD_MSG_CODE_STREAM_ERROR:

                    // NOP
                    break;

                default:

                    createLogMessage(STR_LOG_MSG_FUNC18_CODE_INVAL, LOG_SVRTY_ERR);
                    retval = -1;
                    break;
            }
        }
        pthread_mutex_unlock(&socketFdLock);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC18_ARG_INVAL , LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}