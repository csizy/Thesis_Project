/**
 * @file        common.h
 * @author      Adam Csizy
 * @date        2021-04-13
 * @version     v1.1.0
 * 
 * @brief       Collection of common type and macro definitions
 *              used by both Companion Computer and Ground Control.
 * 
 *              Currently this source file is not used.
 */


#pragma once


#include <pthread.h>
#include <unistd.h>


/* Camera related common macro definitions */

#define NUM_SUP_VID_COD_FMT     7U         /**< Number of supported video coding formats (see VideoCodingFormat_T). */


/* Communication related common macro definitions */

#define LoginMessageField_T     uint32_t    /**< Type of the fields in the login netork message */
#define MessageHeaderField_T    uint32_t    /**< Type of the fields in the header of network messages */
#define VideoStreamPort_T       uint32_t    /**< Type of video streaming port number */

#define NUM_MSG_HEADER_SIZE     2U          /**< Size of message header array in MessageHeaderField_T */
#define IDX_MSG_HEADER_MODULE   0U          /**< Index of module name in message header array */
#define IDX_MSG_HEADER_CODE     1U          /**< Index of module message code in message header array */
#define NUM_LOGIN_MSG_SIZE      2U          /**< Size of login message array in LoginMessageField_T */
#define IDX_LOGIN_MSG_CODE      0U          /**< Index of module message code in login message array */
#define IDX_LOGIN_MSG_ID        1U          /**< Index of drone ID in login message array */


/* Streaming related common macro definitions */

#define NUM_UDP_MTU             64000       /**< MTU for UDP packets in bytes. Theoretical ceiling is 64kB but GStreamer payloaders might not support such a high value.  */


/* Camera related common type definitions */

/**
 * @brief       Enumeration of video coding formats supported
 *              by the GStreamer framework v4l2src element.
 * 
 * @note        This software supports only a subset of the
 *              enumerated formats.
 * 
 *              The enumeration values also define the priorities
 *              of the preferred video coding formats.
 */
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


/* Communication related common type definitions */

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