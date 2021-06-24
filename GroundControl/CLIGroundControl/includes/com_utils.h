/**
 * @file        com_utils.h
 * @author      Adam Csizy
 * @date        2021-04-13
 * @version     v1.1.0
 * 
 * @brief       Ground Control communication utilities
 */


#pragma once


#include <pthread.h>
#include <stdint.h>
#include <unistd.h>


/* Communication related public macro definitions */

#define VideoStreamPort_T       uint32_t /**< Type of video streaming port number */


/* Auxiliary video coding related macro definition */

#define NUM_SUP_VID_COD_FMT     7U /**< Number of supported video coding formats */


/* Auxiliary video coding related public type definition */

typedef enum VideoCodingFormat {

  CAM_FMT_H265    = 0,      /**< H.265 */
  CAM_FMT_H264    = 1,      /**< H.264 */
  CAM_FMT_VP8     = 2,      /**< VP8 */
  CAM_FMT_VP9     = 3,      /**< VP9 */
  CAM_FMT_JPEG    = 4,      /**< JPEG */
  CAM_FMT_H263    = 5,      /**< H.263 */
  CAM_FMT_RAW     = 6,      /**< RAW */
  CAM_FMT_MPEG    = 7,      /**< MPEG (not used) */
  CAM_FMT_MPEGTS  = 8,      /**< MPEGTS (not used) */
  CAM_FMT_BAYER   = 9,      /**< BAYER (not used) */
  CAM_FMT_DV      = 10,     /**< Digital Video (not used) */
  CAM_FMT_FWHT    = 11,     /**< FWHT (not used) */
  CAM_FMT_PWC1    = 12,     /**< PWC1 (not used) */
  CAM_FMT_PWC2    = 13,     /**< PWC2 (not used) */
  CAM_FMT_SONIX   = 14,     /**< Sonix (not used) */
  CAM_FMT_WMV     = 15,     /**< WMV (not used) */
  CAM_FMT_UNK     = 16      /**< Unknown format */

} VideoCodingFormat_T;


/* Communication related public type definitions */

typedef enum ModuleName {

    MOD_NAME_NETWORK            = 1,    /**< Network communication module */
    MOD_NAME_STREAM             = 2,    /**< Video streaming module */
    MOD_NAME_GCCOMMON           = 3     /**< Ground control common module */

} ModuleName_T;

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

typedef union ModuleMessageData {

    VideoCodingFormat_T codingFormat;   /**< Video coding format */
    VideoStreamPort_T videoStreamPort;  /**< Port number on which the video stream is being received  */

} ModuleMessageData_T;

typedef struct ModuleMessage {

    ModuleName_T address;               /**< Target address of module message */
    ModuleMessageCode_T code;           /**< Code of module message */
    ModuleMessageData_T data;           /**< Data of module message */

} ModuleMessage_T;

typedef struct ModuleMessageQueue {

    pthread_mutex_t lock;               /**< Mutex of the circular buffer */
    pthread_cond_t update;              /**< Conditional variable of the circular buffer */
    size_t size;                        /**< Size of the circular buffer */
    size_t front;                       /**< Head of the circular buffer*/
    size_t back;                        /**< Tail of the circular buffer */
    ModuleMessage_T* *messages;         /**< Array of module messages (the buffer itself) */

} ModuleMessageQueue_T;


/* Communication related public function declarations */

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
 * @brief       Initialize services offered by the ground control.
 * 
 * @details     Initializes the server socket and starts
 *              the client handler threads.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int initGroundControlServices(void);
