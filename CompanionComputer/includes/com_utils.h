/**
 * @file        com_utils.h
 * @author      Adam Csizy
 * @date        2021-04-01
 * @version     v1.1.0
 * 
 * @brief       Communication utilities and network module
 */


#pragma once


#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "camera_utils.h" 


/* Communication related public macro definitions */

#define DRONE_ID                12U         /**< Drone ID */
#define VideoStreamPort_T       uint32_t    /**< Type of video streaming port number */
#define MOD_MSGQ_NOBLOCK        1           /**< Module message queue non-blocking flag */
#define MOD_MSGQ_BLOCK          0           /**< Module message queue blocking flag */


/* Communication related public type definitions */

/**
 * @brief   Enumeration of independent modules.
 */
typedef enum ModuleName {

    MOD_NAME_NETWORK            = 1,    /**< Network module (drone) */
    MOD_NAME_STREAM             = 2,    /**< Video streaming module (drone) */
    MOD_NAME_GCCOMMON           = 3     /**< Ground control common module (ground control) */

} ModuleName_T;

/**
 * @brief   Enumeration of module message codes.
 */
typedef enum ModuleMessageCode {

    MOD_MSG_CODE_LOGIN          = 1,    /**< Login to ground control (drone) */
    MOD_MSG_CODE_LOGIN_ACK      = 2,    /**< Login confirmed (ground control) */
    MOD_MSG_CODE_STREAM_REQ     = 3,    /**< Request video stream (ground control) */
    MOD_MSG_CODE_STREAM_ERROR   = 4,    /**< Internal error in video stream (drone) */
    MOD_MSG_CODE_STREAM_START   = 5,    /**< Start video stream (ground control) */
    MOD_MSG_CODE_STREAM_STOP    = 6,    /**< Stop video stream (ground control) */
    MOD_MSG_CODE_STREAM_TYPE    = 7,    /**< Type of requested video stream (drone) */
    MOD_MSG_CODE_LOGIN_NACK     = 8     /**< Login not confirmed (ground control) */

} ModuleMessageCode_T;

/**
 * @brief   Union of module message data.
 */
typedef union ModuleMessageData {

    VideoCodingFormat_T codingFormat;   /**< Video coding format */
    VideoStreamPort_T videoStreamPort;  /**< Port number on which the ground control accepts the video stream  */

} ModuleMessageData_T;

/**
 * @brief   Structure of module message.
 */
typedef struct ModuleMessage {

    ModuleName_T address;               /**< Target address of module message */
    ModuleMessageCode_T code;           /**< Code of module message */
    ModuleMessageData_T data;           /**< Data of module message */

} ModuleMessage_T;

/**
 * @brief       Structure of module message queue.
 * 
 * @details     Each module message queue consists of a
 *              circular buffer a mutex and a conditional
 *              variable to guarantee thread safety.
 */
typedef struct ModuleMessageQueue {

    pthread_mutex_t lock;               /**< Mutex of the circular buffer */
    pthread_cond_t update;              /**< Conditional variable of the circular buffer */
    size_t size;                        /**< Size of the circular buffer */
    size_t front;                       /**< Head of the circular buffer*/
    size_t back;                        /**< Tail of the circular buffer */
    ModuleMessage_T* *messages;         /**< Array of module messages (the buffer itself) */

} ModuleMessageQueue_T;

/**
 * @brief       Structure of network module's initialization context.
 * 
 * @details     A structure holding arguments and parameters for
 *              initializing the network module.
 */
typedef struct NetworkInitContext {

    const char *serverNodeName;         /**< Server address */
    const char *serverServiceName;      /**< Server port */

} NetworkInitContext_T;


/* Communication related global variable declarations */

extern ModuleMessageQueue_T networkMsgq;    /**< Module message queue of the network module */


/* Communication related public function declarations */

/**
 * @brief       Print module message.
 * 
 * @details     Prints module message data on the standard
 *              output.
 * 
 * @param[in]   message Module message object to be printed.
 * .
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int printModuleMessage(const ModuleMessage_T *message);

/**
 * @brief       Initialize module message queue.
 * 
 * @details     Initializes module message queue object. The given
 *              message queue object must be in an uninitialized
 *              or deinitialized state. Double initialization of a
 *              message queue object might result undefined behaviour.
 *              The value of the size input argument must be the
 *              power of two because the implementation uses bitmask
 *              for efficient boundary check.
 * 
 * @note        Not thread safe.
 *              Blocking.
 *
 * @param[in,out]   messageQueue Message queue object to be initialized.
 * @param[in]  size Size of the message queue buffer.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int initModuleMessageQueue(ModuleMessageQueue_T *messageQueue, const size_t size);

/**
 * @brief       Deinitialize module message queue.
 * 
 * @details     Deinitializes module message queue object. The
 *              remaining messages in the queue and the buffer
 *              itself are freed. 
 *
 * @note        Not thread safe.
 *              Blocking.
 *
 * @param[in,out]   messageQueue Message queue object to be deinitialized.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int deinitModuleMessageQueue(ModuleMessageQueue_T *messageQueue);

/**
 * @brief       Insert message into module message queue.
 * 
 * @details     Inserts module message into the given module
 *              message queue (FIFO). The message object must be
 *              dynamically allocated by the source and released
 *              after insertion.
 *
 * @note        Thread safe.
 *
 * @param[in,out]   messageQueue Module message queue object.
 * @param[in]   message Dynamically allocated module message object.
 * @param[in]   noblock Flag whether the call should be non-blocking.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 * @retval      -2 Could not lock queue (prevent blocking)
 * @retval      -3 Message queue is full (prevent blocking)
 */
int insertModuleMessage(ModuleMessageQueue_T *messageQueue, ModuleMessage_T *message, const int noblock);

/**
 * @brief       Remove message from module message queue.
 * 
 * @details     Removes module message from the given module
 *              message queue (FIFO). The message object must be
 *              freed by the receiver.
 *
 * @note        Thread safe.
 *
 * @param[in,out]   messageQueue Module message queue object.
 * @param[out]   message Module message object pointer.
 * @param[in]   noblock Flag whether the call should be non-blocking.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 * @retval      -2 Could not lock queue (prevent blocking)
 * @retval      -3 Message queue is full (prevent blocking)
 */
int removeModuleMessage(ModuleMessageQueue_T *messageQueue, ModuleMessage_T* *message, const int noblock);

/**
 * @brief       Wrapper for 'recv()' with timeout option.
 * 
 * @details     This function serves as a wrapper for function
 *              'recv()' with timeout option.
 *
 * @param[in]   sockfd Socket file descriptor.
 * @param[out]   buf Received data buffer.
 * @param[in]   len Size of the data to be received in bytes.
 * @param[in]   flags Flags.
 * @param[in]   sec Timeout interval in seconds.
 * @param[in]   usec Timeout interval in microseconds.
 * 
 * @return      The number of bytes received, or -1 if an error
 *              occurred. In the event of an error, errno is set
 *              to indicate the error.
 */
ssize_t recvTimeout(int sockfd, void *buf, size_t len, int flags, time_t sec, useconds_t usec);

/**
 * @brief       Initialize network handler module.
 * 
 * @details     Initializes the network handler module's
 *              message queue and starts the network traffic
 *              handler threads.
 * 
 * @param[in]   initCtx Initialization context.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int initNetworkModule(NetworkInitContext_T *initCtx);