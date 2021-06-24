/**
 * @file        log_utils.h
 * @author      Adam Csizy
 * @date        2021-04-13
 * @version     v1.1.0
 * 
 * @brief       Logging utilities
 */


#pragma once


/* Log related public macro definitions */

#define STR_LOG_MSG_UNEXP_SVRTY_LVL             "createLogMessage(): Unexpected log message severity level."

#define STR_LOG_MSG_FUNC1_SRVR_START_FAIL       "initCommunicationModule(): Failed to start server."
#define STR_LOG_MSG_FUNC1_THRD_START_FAIL       "initCommunicationModule(): Failed to start drone service threads."

#define STR_LOG_MSG_FUNC2_SOCK_CREAT_FAIL       "startServer(): Failed to create server socket."
#define STR_LOG_MSG_FUNC2_SOCK_CONF_FAIL        "startServer(): Failed to configure server socket."
#define STR_LOG_MSG_FUNC2_SOCK_BIND_FAIL        "startServer(): Failed to bind server socket to server address."
#define STR_LOG_MSG_FUNC2_SOCK_LISTEN_FAIL      "startServer(): Failed to set server socket to passive mode."

#define STR_LOG_MSG_FUNC3_THRD_CREAT_FAIL       "[ERROR] startDroneServiceThreads(): Failed to create drone service thread with ID %d.\n"

#define STR_LOG_MSG_FUNC4_CONN_ACCEPT_FAIL      "[WARNING] threadFuncDroneService(): Thread %d failed to accept connection request.\n"
#define STR_LOG_MSG_FUNC4_SOCK_CONF_FAIL        "[WARNING] threadFuncDroneService(): Thread %d failed to configure service socket.\n"
#define STR_LOG_MSG_FUNC4_DRONE_AUTH_FAIL       "[WARNING] threadFuncDroneService(): Thread %d failed to authenticate drone with ID <%d>.\n"
#define STR_LOG_MSG_FUNC4_DRONE_AUTH_SUCCESS    "[INFO] threadFuncDroneService(): Thread %d succeeded to authenticate drone with ID <%d>.\n"
#define STR_LOG_MSG_FUNC4_CONN_CLOSED           "[WARNING] threadFuncDroneService(): Connection lost or closed by the drone in thread %d.\n"
#define STR_LOG_MSG_FUNC4_MSG_HANDLE_FAIL       "[WARNING] threadFuncDroneService(): Thread %d failed to handle drone message."
#define STR_LOG_MSG_FUNC4_CLI_HANDLE_FAIL       "[WARNING] threadFuncDroneService(): Thread %d failed to handle CLI input."
#define STR_LOG_MSG_FUNC4_DRONE_ADDR_RES        "[INFO] threadFuncDroneService(): Thread %d accepted drone connection from IP <%s> PORT <%s>.\n"
#define STR_LOG_MSG_FUNC4_DRONE_ADDR_RES_FAIL   "[INFO] threadFuncDroneService(): Thread %d accepted drone connection. Drone address could not be resolved. Reason: %s.\n"

#define STR_LOG_MSG_FUNC5_ARG_INVAL             "authDrone(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC5_LOGIN_RECV_FAIL       "authDrone(): Failed to receive login message or response timed out."
#define STR_LOG_MSG_FUNC5_LOGIN_SEND_FAIL       "authDrone(): Failed to send login message."

#define STR_LOG_MSG_FUNC6_ARG_INVAL             "pipeBuilder(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC6_CREAT_ELEM_FAIL       "pipeBuilder(): Failed to create pipeline element(s)."
#define STR_LOG_MSG_FUNC6_PIPE_LINK_FAIL        "pipeBuilder(): Failed to link pipeline elements."
#define STR_LOG_MSG_FUNC6_PIPE_SET_INIT_FAIL    "pipeBuilder(): Failed to set pipeline to its initial state."
#define STR_LOG_MSG_FUNC6_MAIN_LOOP_START_FAIL  "pipeBuilder(): Failed to start GStreamer main loop thread."
#define STR_LOG_MSG_FUNC6_FMT_INVAL             "pipeBuilder(): Invalid video coding format."

#define STR_LOG_MSG_FUNC7_GST_INIT_FAIL         "initStreamModule(): Failed to initialize GStreamer core and its plugins."

#define STR_LOG_MSG_FUNC8_ARG_INVAL             "inputMessageHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC8_MSG_RECV_FAIL         "inputMessageHandler(): Failed to receive module message or response timed out."
#define STR_LOG_MSG_FUNC8_MSG_RECV_INVAL        "inputMessageHandler(): Invalid module message received."
#define STR_LOG_MSG_FUNC8_STRM_STOP_FAIL        "inputMessageHandler(): Failed to stop ground control video display pipeline."

#define STR_LOG_MSG_FUNC9_ARG_INVAL             "inputCommandHandler(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC9_REQ_STRM_FAIL         "inputCommandHandler(): Failed to accomplish user command 'play'."
#define STR_LOG_MSG_FUNC9_STOP_STRM_FAIL        "inputCommandHandler(): Failed to accomplish user command 'stop'."

#define STR_LOG_MSG_FUNC10_ARG_INVAL            "stopStream(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC10_PIPE_SET_INIT_FAIL   "stopStream(): Failed to set pipeline to its initial state."

#define STR_LOG_MSG_FUNC11_ARG_INVAL            "sendStopMessage(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC11_MSG_SEND_FAIL        "sendStopMessage(): Failed to send module message."

#define STR_LOG_MSG_FUNC12_ARG_INVAL            "requestStream(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC12_MSG_REQ_SEND_FAIL    "requestStream(): Failed to send REQUEST STREAM module message header."
#define STR_LOG_MSG_FUNC12_MSG_PORT_SEND_FAIL   "requestStream(): Failed to send RTP video stream destination port."
#define STR_LOG_MSG_FUNC12_MSG_TYP_RECV_FAIL    "requestStream(): Failed to receive STREAM TYPE module message header or response timed out."
#define STR_LOG_MSG_FUNC12_MSG_FMT_RECV_FAIL    "requestStream(): Failed to receive video stream coding format or response timed out."
#define STR_LOG_MSG_FUNC12_MSG_TYP_INVAL        "requestStream(): Invalid STREAM TYPE module message code."
#define STR_LOG_MSG_FUNC12_MSG_START_SEND_FAIL  "requestStream(): Failed to send STREAM START module message header."
#define STR_LOG_MSG_FUNC12_PIPE_SET_PLAY_FAIL   "requestStream(): Failed to set video display pipeline to PLAYING state."
#define STR_LOG_MSG_FUNC12_PIPE_BUILD_FAIL      "requestStream(): Failed to build video display pipeline."

#define STR_LOG_MSG_FUNC13_ARG_INVAL            "waitPipeStateChange(): Invalid input argument(s)."
#define STR_LOG_MSG_FUNC13_PIPE_ERROR           "waitPipeStateChange(): Error occured while waiting for state change."
#define STR_LOG_MSG_FUNC13_MSG_UNEXP            "waitPipeStateChange(): Received an unexpected pipeline message."
#define STR_LOG_MSG_FUNC13_PIPE_WAIT_FAIL       "waitPipeStateChange(): Failed to wait for state change completion."

#define STR_LOG_MSG_FUNC14_PIPE_ERROR           "pipelineErrorCallback(): Error received from pipeline."

#define STR_LOG_MSG_FUNC15_LOOP_CREAT_FAIL      "threadFuncStreamMainLoop(): Failed to create main loop."

#define STR_LOG_MSG_MAIN_PROG_STARTUP           "main(): Ground Control launched!"
#define STR_LOG_MSG_MAIN_SERVER_INIT_FAIL       "main(): Failed to initialize and launch ground control services."
#define STR_LOG_MSG_MAIN_STREAM_INIT_FAIL       "main(): Failed to initialize streaming services."


/* Log related public type definitions */

typedef enum LogSeverity {

    LOG_SVRTY_ERR   =   0,  /**< Error LOG severity level */
    LOG_SVRTY_WRN   =   1,  /**< Warning LOG severity level */
    LOG_SVRTY_INF   =   2,  /**< Information LOG severity level */
    LOG_SVRTY_DBG   =   3   /**< Debug LOG severity level */

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