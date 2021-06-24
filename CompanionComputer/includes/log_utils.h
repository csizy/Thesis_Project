/**
 * @file        log_utils.h
 * @author      Adam Csizy
 * @date        2021-03-19
 * @version     v1.1.0
 * 
 * @brief       Logging utilities
 */


#pragma once


/* Log related public macro definitions */

#define STR_LOG_MSG_UNEXP_SVRTY_LVL             "createLogMessage(): Unexpected log message severity level."
#define STR_LOG_MSG_FUNC1_ARG_INVAL             "getCameraDevicePath(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC1_OPEN_DIR_FAIL         "getCameraDevicePath(): Failed to open device directory."
#define STR_LOG_MSG_FUNC1_OPEN_DEV_FAIL         "[WARNING] getCameraDevicePath(): Failed to open video device: %s.\n"
#define STR_LOG_MSG_FUNC1_QUERY_CAP_FAIL        "[WARNING] getCameraDevicePath(): Failed to query video device capabilities. Device (%s) might not support V4L2 interface.\n"

#define STR_LOG_MSG_FUNC2_ARG_INVAL             "stringToVideoCodingFormat(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC3_ARG_INVAL             "printVideoCodingFormatCaps(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC4_OUT_OF_RANGE          "retrieveVideoCodingFormatCap(): Video coding format is out of range."

#define STR_LOG_MSG_FUNC5_ARG_INVAL             "getCameraCapabilities(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC5_CAPS_ANY              "getCameraCapabilities(): Camera device has ANY video coding format capabilities."
#define STR_LOG_MSG_FUNC5_CAPS_EMPTY            "getCameraCapabilities(): Camera device has EMPTY set of video coding format capbilities."
#define STR_LOG_MSG_FUNC5_SRCPAD_RTRV_FAIL      "getCameraCapabilities(): Failed to retrieve source pad of camera device pipeline element."
#define STR_LOG_MSG_FUNC5_CAPS_RTRV_FAIL        "getCameraCapabilities(): Failed to retrieve capabilities of source pad."

#define STR_LOG_MSG_FUNC7_ARG_INVAL             "initModuleMessageQueue(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC7_MEM_ALLOC_FAIL        "initModuleMessageQueue(): Failed to allocate memory for message queue buffer."
#define STR_LOG_MSG_FUNC7_MTX_ATTR_INIT_FAIL    "initModuleMessageQueue(): Failed to initialize mutex attribute."
#define STR_LOG_MSG_FUNC7_MTX_ATTR_SET_TYPE_FAIL "initModuleMessageQueue(): Failed to set type of mutex attribute."
#define STR_LOG_MSG_FUNC7_MTX_INIT_FAIL         "initModuleMessageQueue(): Failed to initialize mutex."
#define STR_LOG_MSG_FUNC7_COND_ATTR_INIT_FAIL   "initModuleMessageQueue(): Failed to initialize conditional variable attribute."
#define STR_LOG_MSG_FUNC7_COND_INIT_FAIL        "initModuleMessageQueue(): Failed to initialize conditional variable."

#define STR_LOG_MSG_FUNC8_ARG_INVAL             "deinitModuleMessageQueue(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC8_MTX_DSTRY_FAIL        "deinitModuleMessageQueue(): Failed to destroy mutex."
#define STR_LOG_MSG_FUNC8_COND_DSTRY_FAIL       "deinitModuleMessageQueue(): Failed to destroy conditional variable."

#define STR_LOG_MSG_FUNC9_ARG_INVAL             "insertModuleMessage(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC10_ARG_INVAL            "removeModuleMessage(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC11_ARG_INVAL            "printModuleMessage(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC12_ARG_INVAL            "connectToGroundControl(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC12_GC_ADDR_RESLV_FAIL   "connectToGroundControl(): Failed to resolve address of ground control."
#define STR_LOG_MSG_FUNC12_GETADDRINFO_FAIL     "[ERROR] connectToGroundControl(): getaddrinfo(): %s\n"
#define STR_LOG_MSG_FUNC12_GC_NOT_FOUND         "connectToGroundControl(): No ground control found with the given parameters."
#define STR_LOG_MSG_FUNC12_CREAT_SOCK_FAIL      "connectToGroundControl(): Failed to create socket."
#define STR_LOG_MSG_FUNC12_GC_CONN_FAIL         "connectToGroundControl(): Failed to estabilish connection with ground control."
#define STR_LOG_MSG_FUNC12_SET_KEEPALIVE_FAIL   "connectToGroundControl(): Failed to set keep alive on socket."
#define STR_LOG_MSG_FUNC12_GC_CONN_SUCCESS      "[INFO] connectToGroundControl(): Successfully estabilished connection with ground control (%s:%s).\n"
#define STR_LOG_MSG_FUNC12_LOGIN_SEND_FAIL      "connectToGroundControl(): Failed to send login message to ground control."
#define STR_LOG_MSG_FUNC12_LOGIN_ACK_INVAL      "connectToGroundControl(): Invalid login message acknowledgement from ground control."
#define STR_LOG_MSG_FUNC12_LOGIN_RECV_FAIL      "connectToGroundControl(): Failed to receive login message from ground control."

#define STR_LOG_MSG_FUNC13_THRD_START_FAIL      "threadFuncNetworkIn(): Failed to start network output handler thread."
#define STR_LOG_MSG_FUNC13_GC_CONN_CLOSED       "threadFuncNetworkIn(): Connection lost/closed to ground control."
#define STR_LOG_MSG_FUNC13_CONN_FAIL_RETRY      "[WARNING] threadFuncNetworkIn(): Failed to connect to ground control. Retrying after %d seconds.\n"
#define STR_LOG_MSG_FUNC13_RECONN_FAIL_RETRY    "[WARNING] threadFuncNetworkIn(): Failed to reconnect to ground control. Retrying after %d seconds.\n"

#define STR_LOG_MSG_FUNC14_MSG_RMV_FAIL         "threadFuncNetworkOut(): Failed to remove message from network module's message queue."

#define STR_LOG_MSG_FUNC15_ARG_INVAL            "inputMessageHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC15_HDR_RECV_FAIL        "inputMessageHandler(): Failed to receive message header."
#define STR_LOG_MSG_FUNC15_MOD_NAME_INVAL       "inputMessageHandler(): Invalid module name in message header."
#define STR_LOG_MSG_FUNC15_PROC_MSG_STRM_FAIL   "inputMessageHandler(): Failed to process stream module message."

#define STR_LOG_MSG_FUNC16_ARG_INVAL            "networkToStreamMessage(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC16_MSG_ALLOC_FAIL       "networkToStreamMessage(): Failed to allocate module message object."
#define STR_LOG_MSG_FUNC16_CODE_INVAL           "networkToStreamMessage(): Invalid module message code."
#define STR_LOG_MSG_FUNC16_STRM_PORT_RECV_FAIL  "networkToStreamMessage(): Failed to receive video stream port."

#define STR_LOG_MSG_FUNC17_MOD_NAME_INVAL       "threadFuncNetworkOut(): Invalid module name."
#define STR_LOG_MSG_FUNC17_PROC_MSG_CMN_FAIL    "threadFuncNetworkOut(): Failed to process ground control common module message."

#define STR_LOG_MSG_FUNC18_ARG_INVAL            "gccommonMessageToNetwork(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC18_CODE_INVAL           "gccommonMessageToNetwork(): Invalid module message code."
#define STR_LOG_MSG_FUNC18_HDR_SEND_FAIL        "gccommonMessageToNetwork(): Failed to send message header."
#define STR_LOG_MSG_FUNC18_DATA_SEND_FAIL       "gccommonMessageToNetwork(): Failed to send message data."

#define STR_LOG_MSG_FUNC19_MSGQ_INIT_FAIL       "initNetworkModule(): Failed to initialize network module's message queue."
#define STR_LOG_MSG_FUNC19_THRD_IN_START_FAIL   "initNetworkModule(): Failed to start network input handler thread."

#define STR_LOG_MSG_FUNC20_MSGQ_INIT_FAIL       "initStreamModule(): Failed to initialize streaming module's message queue."
#define STR_LOG_MSG_FUNC20_THRD_CTRL_START_FAIL "initStreamModule(): Failed to start stream control thread."
#define STR_LOG_MSG_FUNC20_GST_INIT_FAIL        "initStreamModule(): Failed to initialize GStreamer."

#define STR_LOG_MSG_FUNC21_MSG_RMV_FAIL         "threadFuncStreamControl(): Failed to remove message from streaming module's message queue."
#define STR_LOG_MSG_FUNC21_CODE_INVAL           "threadFuncStreamControl(): Invalid module message code."
#define STR_LOG_MSG_FUNC21_CAMDEV_NOT_FOUND     "threadFuncStreamControl(): Failed to detect compatible camera device."
#define STR_LOG_MSG_FUNC21_CAM_CAPS_INIT_FAIL   "threadFuncStreamControl(): Failed to initialize camera capabilities."
#define STR_LOG_MSG_FUNC21_PIPE_BUILD_FAIL      "threadFuncStreamControl(): Failed to build video streaming pipeline."
#define STR_LOG_MSG_FUNC21_REG_CBS_FAIL         "threadFuncStreamControl(): Failed to register callback functions."
#define STR_LOG_MSG_FUNC21_THRD_START_FAIL      "threadFuncStreamControl(): Failed to start main loop thread."

#define STR_LOG_MSG_FUNC22_ARG_INVAL            "initCameraCapabilities(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC22_CREAT_ELEM_FAIL      "initCameraCapabilities(): Failed to create pipeline element(s)."
#define STR_LOG_MSG_FUNC22_PIPE_STATE_SET_FAIL  "initCameraCapabilities(): Failed to set pipeline state."
#define STR_LOG_MSG_FUNC22_PIPE_ELEM_ERROR_MSG  "[ERROR] initCameraCapabilities(): Error received from element %s: %s.\n"
#define STR_LOG_MSG_FUNC22_PIPE_ELEM_ERROR_DBG  "[ERROR] initCameraCapabilities(): Debugging information: %s.\n"
#define STR_LOG_MSG_FUNC22_CAM_CAPS_GET_FAIL    "initCameraCapabilities(): Failed to get camera capabilities."
#define STR_LOG_MSG_FUNC22_MSG_UNEXP            "initCameraCapabilities(): Unexpected message received."

#define STR_LOG_MSG_FUNC30_ARG_INVAL            "pipeBuilder(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC30_CREAT_ELEM_FAIL      "pipeBuilder(): Failed to create pipeline element(s)."
#define STR_LOG_MSG_FUNC30_PIPE_LINK_FAIL       "pipeBuilder(): Failed to link pipeline elements."
#define STR_LOG_MSG_FUNC30_PIPE_SET_INIT_FAIL   "pipeBuilder(): Failed to set pipeline to its initial state."
#define STR_LOG_MSG_FUNC30_CODING_FMT_INVAL     "pipeBuilder(): Invalid video coding format."
#define STR_LOG_MSG_FUNC30_PIPE_TYPE_INFO       "[INFO] pipeBuilder(): Constructed video streaming pipeline using %s camera output format.\n"

#define STR_LOG_MSG_FUNC31_ARG_INVAL            "pipelineErrorCallback(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC31_PIPE_ELEM_ERROR_MSG  "[INFO] pipelineErrorCallback(): Error received from element %s: %s.\n"
#define STR_LOG_MSG_FUNC31_PIPE_ELEM_ERROR_DBG  "[INFO] pipelineErrorCallback(): Debugging information: %s.\n"
#define STR_LOG_MSG_FUNC31_MSG_ALLOC_FAIL       "pipelineErrorCallback(): Failed to allocate module message object."

#define STR_LOG_MSG_FUNC32_PIPE_EOS             "pipelineEosCallback(): End of videostream (EOS) detected."
#define STR_LOG_MSG_FUNC32_MSG_ALLOC_FAIL       "pipelineEosCallback(): Failed to allocate module message object."

#define STR_LOG_MSG_FUNC33_ARG_INVAL            "pipelineStatechangedCallback(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC33_PIPE_STATE_CHANGE    "[INFO] pipelineStatechangedCallback(): Pipeline state changed from %s to %s.\n"

#define STR_LOG_MSG_FUNC34_ARG_INVAL            "registerCallbackFunctions(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC35_LOOP_CREAT_FAIL      "threadFuncStreamMainLoop(): Failed to create a GMainLoop object."

#define STR_LOG_MSG_FUNC36_ARG_INVAL            "emptyHandler(): Invalid input argument(s)."

#define STR_LOG_MSG_FUNC37_ARG_INVAL            "streamRequestHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC37_MSG_ALLOC_FAIL       "streamRequestHandler(): Failed to allocate module message object."
#define STR_LOG_MSG_FUNC37_PORT_SET_FAIL        "streamRequestHandler(): Failed to set network sink element's port property."

#define STR_LOG_MSG_FUNC38_ARG_INVAL            "streamStopHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC38_PIPE_SET_INIT_FAIL   "streamStopHandler(): Failed to set pipeline to its initial state."
#define STR_LOG_MSG_FUNC38_SM_STATE_INCON       "streamStopHandler(): State machine might enter into an inconsistent state."

#define STR_LOG_MSG_FUNC39_ARG_INVAL            "streamStartHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC39_PIPE_SET_PLAY_FAIL   "streamStartHandler(): Failed to set pipeline to PLAYING state."
#define STR_LOG_MSG_FUNC39_SM_STATE_INCON       "streamStartHandler(): State machine might enter into an inconsistent state."

#define STR_LOG_MSG_FUNC40_ARG_INVAL            "streamErrorHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC40_PIPE_SET_NULL_FAIL   "streamErrorHandler(): Failed to set pipeline to NULL state."
#define STR_LOG_MSG_FUNC40_SM_STATE_INCON       "streamErrorHandler(): State machine might enter into an inconsistent state."

#define STR_LOG_MSG_FUNC41_ARG_INVAL            "videoCodingFormatToString(): Invalid input argument(s)."

#define STR_LOG_MSG_MAIN_PROG_STARTUP           "main(): Streamer program launched!"
#define STR_LOG_MSG_MAIN_MOD_NET_INIT_FAIL      "main(): Failed to initialize and start network module."
#define STR_LOG_MSG_MAIN_MOD_STRM_INIT_FAIL     "main(): Failed to initialize and start streaming module."
#define STR_LOG_MSG_MAIN_CTX_NET_ALLOC_FAIL     "main(): Failed to allocate network module's initialization context."

/* Log related public type definitions */

/**
 * @brief   Enumeration of log severity levels.
 */
typedef enum LogSeverity {

    LOG_SVRTY_ERR   =   0,  /**< Error */
    LOG_SVRTY_WRN   =   1,  /**< Warning */
    LOG_SVRTY_INF   =   2,  /**< Information */
    LOG_SVRTY_DBG   =   3   /**< Debug information */

} LogSeverity_T;


/* Log related public function declarations */

/**
 * @brief       Create log message.
 * 
 * @details     Generates a log entry in the system log
 *              and prints the log message on the standard
 *              output with the given severity level if
 *              debug mode is enabled.
 * 
 * @note        None
 * 
 * @param[in]   message Log message.
 * @param[in]   severity Log severity level.
 */
void createLogMessage(const char message[], const LogSeverity_T severity);