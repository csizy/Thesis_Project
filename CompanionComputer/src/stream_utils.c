/**
 * @file        stream_utils.c
 * @author      Adam Csizy
 * @date        2021-04-05
 * @version     v1.1.0
 * 
 * @brief       Streaming utilities module
 */


#include <gst/gst.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "camera_utils.h"
#include "com_utils.h"
#include "log_utils.h"
#include "stream_utils.h"


/* Streaming related macro definitions */

#define NUM_STREAM_MSGQ_SIZE        8U  /**< Size of streaming module's message queue */
#define NUM_STREAM_STATE_NUM        2U  /**< Number of stream states */
#define NUM_STREAM_EVENT_NUM        4U  /**< Number of stream events */
#define SM_UPDATE_REQUIRED          1U  /**< State machine update required */
#define SM_UPDATE_NOT_REQUIRED      0U  /**< State machine update not required */
#define NUM_CAM_DEV_PATH_SIZE       64U /**< Camera device path size */
//#define STR_STREAM_DEST_ADDR        "195.441.0.134" /**< Default address of RTP stream destination (LAN) */
#define STR_STREAM_DEST_ADDR        "any_custom_domain.ddns.net" /**< Default address of RTP stream destination (WAN) */
//#define STR_STREAM_DEST_PORT        "5000" /**< Default service port of RTP stream destination (LAN) */
#define STR_STREAM_DEST_PORT        "17000" /**< Default service port of RTP stream destination (WAN) */
//#define NUM_STREAM_DEST_PORT        5000 /**< Default service port of RTP stream destination (LAN) */
#define NUM_STREAM_DEST_PORT        17000 /**< Default service port of RTP stream destination (WAN) */
#define PIPE_INITIAL_STATE          GST_STATE_READY /**< Initial state of the video streaming pipeline */
#define NUM_UDP_MTU                 64000 /**< MTU for UDP packets in bytes. Theoretical ceiling is 64kB but GStreamer payloaders might not support such a high value.  */
#define STR_PIPE_ELEM_NAME_VIDSRC   "Video_Source" /**< Name of the video source pipeline element */
#define STR_PIPE_ELEM_NAME_VIDCONV  "Video_Converter" /**< Name of the video converter pipeline element */
#define STR_PIPE_ELEM_NAME_CAPSFLTR "Video_Caps_Filter" /**< Name of the video capabilities-filter pipeline element */
#define STR_PIPE_ELEM_NAME_ENCODER  "Video_Encoder" /**< Name of the video encoder pipeline element */
#define STR_PIPE_ELEM_NAME_PAYLDR   "Payloader" /**< Name of the payloader pipeline element */
#define STR_PIPE_ELEM_NAME_NETSINK  "Network_Sink" /**< Name of the network sink pipeline element */

/* Streaming related static type declarations */

typedef void* (*EventHandler_T)(ModuleMessage_T* *message, GstElement* *pipeline); 

/**
 * @brief   Enumeration of video streaming events.
 */
typedef enum StreamEvent {

    STREAM_EVENT_STREAM_REQ     = 0,    /**< Ground control requested video stream (type) */
    STREAM_EVENT_STREAM_START   = 1,    /**< Ground control requested to start video stream */
    STREAM_EVENT_STREAM_STOP    = 2,    /**< Ground control requested to stop video stream */
    STREAM_EVENT_PIPE_ERROR     = 3     /**< Error occured in streaming pipeline */

} StreamEvent_T;

/**
 * @brief   Enumeration of video streaming states.
 */
typedef enum StreamState {

    STREAM_STATE_STANDBY        = 0,    /**< Pipeline in standby state */
    STREAM_STATE_PLAYING        = 1,    /**< Pipeline in playing state */

} StreamState_T;

/**
 * @brief   Struct of stream state context.
 */
typedef struct StateContext {

    StreamState_T nextState;
    EventHandler_T eventHandler;

} StateContext_T;


/* Streaming related global variable declarations */

ModuleMessageQueue_T streamMsgq;


/* Streaming related static variable declarations */

static pthread_t threadStreamControl;   /**< Thread object for handling video stream state machine */
static pthread_t threadStreamMainLoop;  /**< Thread object for handling main loop context of the video stream */
static VideoCodingFormat_T currentCodingFormat = CAM_FMT_UNK;   /**< Current coding format used by the video streaming pipeline */


/* Streaming related static function declarations */

/**
 * @brief       Start routine of stream controller thread.
 * 
 * @details     Initializes and controls video streaming media
 *              pipeline on a state machine basis. This function
 *              also responsible for handling the streaming
 *              module's message traffic
 * 
 * @param[in]   arg Launch argument (not used).
 * 
 * @return      Any (not used).
 */
static void* threadFuncStreamControl(void *arg);

/**
 * @brief       Start routine of stream main loop thread.
 * 
 * @details     Initializes and starts a GMainLoop object
 *              using the default context. In terms of the
 *              video streaming application the main loop is
 *              responsible for periodically checking the
 *              pipeline's bus and emitting the asynchronous
 *              message signals. The registered signal callback
 *              functions are invoked in the main loop's thread
 *              context (i.e. in this thread context).
 *              
 * 
 * @param[in]   arg Launch argument (not used).
 * 
 * @return      Any (not used).
 */
static void* threadFuncStreamMainLoop(void *arg);

/**
 * @brief       Initialize stream controller.
 * 
 * @details     Initializes the given stream controller state machine.
 *              
 * @param[in]   controller Stream controller to be initialized.
 */
static void initStreamController(StateContext_T controller[NUM_STREAM_STATE_NUM][NUM_STREAM_EVENT_NUM]);

/**
 * @brief       Empty event handler.
 * 
 * @details     Event handler for events which require no action.
 *              The given module message is freed.
 *              
 * @param[in,out]   message Module message.
 * @param[in,out]   pipeline GStreamer video streaming pipeline.
 * 
 * @return      Any (not used).
 */
static void* emptyHandler(ModuleMessage_T* *message, GstElement* *pipeline);

/**
 * @brief       Stream request event handler.
 * 
 * @details     Event handler for stream request events. The
 *              stream's video coding format is sent back to
 *              the ground control and the given module message
 *              is freed. The 'port' property of the network sink
 *              element is set according to the request message
 *              data.
 *              
 * @param[in,out]   message Module message.
 * @param[in,out]   pipeline GStreamer video streaming pipeline.
 * 
 * @return      Any (not used).
 */
static void* streamRequestHandler(ModuleMessage_T* *message, GstElement* *pipeline);

/**
 * @brief       Stream stop event handler.
 * 
 * @details     Event handler for stream stop events. Stops
 *              the video streaming and sets the pipeline 
 *              back to its initial state. The given module
 *              message is freed.
 *              
 * @param[in,out]   message Module message.
 * @param[in,out]   pipeline GStreamer video streaming pipeline.
 * 
 * @return      Any (not used).
 */
static void* streamStopHandler(ModuleMessage_T* *message, GstElement* *pipeline);

/**
 * @brief       Stream start event handler.
 * 
 * @details     Event handler for stream start events. Starts
 *              the video streaming and frees the given module
 *              message.
 *              
 * @param[in,out]   message Module message.
 * @param[in,out]   pipeline GStreamer video streaming pipeline.
 * 
 * @return      Any (not used).
 */
static void* streamStartHandler(ModuleMessage_T* *message, GstElement* *pipeline);

/**
 * @brief       Stream error event handler.
 * 
 * @details     Event handler for stream error events. On errors
 *              coming from the GStreamer pipeline elements the
 *              video streaming is stopped and the pipeline is
 *              set back to NULL state resulting an internal
 *              state reset for each pipeline component. The given
 *              module message is forwarded to the ground control
 *              over the network module.
 *              
 * @param[in,out]   message Module message.
 * @param[in,out]   pipeline GStreamer video streaming pipeline.
 * 
 * @return      Any (not used).
 */
static void* streamErrorHandler(ModuleMessage_T* *message, GstElement* *pipeline);

/**
 * @brief       Initializes camera capabilities.
 * 
 * @details     Initializes camera capabilities array in the
 *              given initialization context with capabilities
 *              of the camera device under path 'camDevPath'.
 * 
 * @note        GStreamer core and plugins must be initialized
 *              using 'gst_init()' before invoking this function.
 * 
 * @param[in]   camDevPath Path to camera device.
 * @param[in]   initCtx Capabilities's initialization context.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int initCameraCapabilities(const char *camDevPath, VideoCodingFormatContext_T *initCtx);

/**
 * @brief       Build media pipeline.
 * 
 * @details     Builds a GStreamer media pipeline compatible with
 *              the given video coding format as the camera
 *              device's output. The given capabilities argument
 *              contains optimal capability configurations for
 *              each supported video coding format and is used to
 *              enhance the video stream quality. The video stream
 *              is forwarded over UDP/RTP to the ground control.
 * 
 * @note        GStreamer core and plugins must be initialized
 *              using 'gst_init()' before invoking this function.
 *
 * @param[in,out]   pipeline Pointer to a pipeline to be built.
 * @param[in]   camDevPath Path to camera device.
 * @param[in]   codingFormat Video encoding format.
 * @param[in]   caps Video coding capabilities.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int pipeBuilder(GstElement* *pipeline, const char *camDevPath, const VideoCodingFormat_T codingFormat, const VideoCodingFormatCaps_T caps[NUM_SUP_VID_COD_FMT]);

/**
 * @brief       Pipeline error signal callback.
 * 
 * @details     Callback function for handling pipeline error
 *              signals. On pipeline error a log record is created
 *              and the stream controller is notified using the
 *              streaming module's message queue.
 *
 * @param[in,out]   bus Pipeline's bus.
 * @param[in]   message Pipeline error message.
 * @param[in]   data Custom data passed to the callback function (not used).
 */
static void pipelineErrorCallback(GstBus *bus, GstMessage *message, gpointer data);

/**
 * @brief       Pipeline end-of-stream (EOS) signal callback.
 * 
 * @details     Callback function for handling end-of-stream signals.
 *              On EOS signal a log record is created and the stream
 *              controller is notified using the streaming module's
 *              message queue.
 *
 * @param[in,out]   bus Pipeline's bus.
 * @param[in]   message Pipeline EOS message.
 * @param[in]   data Custom data passed to the callback function (not used).
 */
static void pipelineEosCallback(GstBus *bus, GstMessage *message, gpointer data);

/**
 * @brief       Pipeline state-changed signal callback.
 * 
 * @details     Callback function for handling the pipeline's state
 *              changed signal. On state changed event a log record is
 *              created. This callback function only handles state
 *              changed messages coming from the pipeline itself. For
 *              such filtering a pointer to the pipeline is given to
 *              detect the appropriate message source.
 * 
 * @note        Intended for debug purposes.
 *
 * @param[in,out]   bus Pipeline's bus.
 * @param[in]   message Pipeline state changed message.
 * @param[in]   data Pointer to the pipeline.
 */
static void pipelineStatechangedCallback(GstBus *bus, GstMessage *message, gpointer data);

/**
 * @brief       Register signal callback functions.
 * 
 * @details     Registers signal callback functions at the
 *              given GStreamer pipeline's bus to handle the
 *              occurance of different events coming from the
 *              pipeline elements. By invoking this function
 *              a bus signal watch is also being added to the
 *              default main context.
 * 
 * @note        On cleanup 'gst_bus_remove_signal_watch()' should
 *              be called to remove the signal watch from the main
 *              context.
 *
 * @param[in,out]   pipeline GStreamer pipeline.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int registerCallbackFunctions(GstElement *pipeline);


/* Streaming related function definitions */

int initStreamModule(void) {

    int retval = 0;

    if(FALSE == gst_init_check(NULL, NULL, NULL)) {

        createLogMessage(STR_LOG_MSG_FUNC20_GST_INIT_FAIL, LOG_SVRTY_ERR);

        retval = -1;
        return retval;
    }

    if(initModuleMessageQueue(&streamMsgq, NUM_STREAM_MSGQ_SIZE)) {

        createLogMessage(STR_LOG_MSG_FUNC20_MSGQ_INIT_FAIL, LOG_SVRTY_ERR);
        
        retval = -1;
        return retval;
    }

    if(pthread_create(&threadStreamControl, NULL, threadFuncStreamControl, NULL)) {

        createLogMessage(STR_LOG_MSG_FUNC20_THRD_CTRL_START_FAIL, LOG_SVRTY_ERR);

        deinitModuleMessageQueue(&streamMsgq);
        retval = -1;
        return retval;
    }

    return retval;
}

static void* threadFuncStreamControl(void *arg) {

    /* Stream control related variables */

    int updateRequired = SM_UPDATE_NOT_REQUIRED;
    StreamEvent_T event;
    StreamState_T state = STREAM_STATE_STANDBY;
    StateContext_T streamController[NUM_STREAM_STATE_NUM][NUM_STREAM_EVENT_NUM] = {0};
    ModuleMessage_T *message = NULL;

    /* Pipeline related variables */

    int i, errorCode, built;
    char camDevPath[NUM_CAM_DEV_PATH_SIZE] = {0};
    GstElement *pipeline = NULL;
    VideoCodingFormatCaps_T cameraCapabilities[NUM_SUP_VID_COD_FMT] = {0};
    VideoCodingFormatContext_T context = {

        .capsArray = cameraCapabilities,
        .size = NUM_SUP_VID_COD_FMT
    };

    /* Detect compatible camera device */
    if(getCameraDevicePath(camDevPath, NUM_CAM_DEV_PATH_SIZE)) {

        createLogMessage(STR_LOG_MSG_FUNC21_CAMDEV_NOT_FOUND, LOG_SVRTY_ERR);
        kill(getpid(), SIGTERM);
        pthread_exit(NULL);
    }

    /* Initialize camera device capabilities */
    if(initCameraCapabilities(camDevPath, &context)) {

        createLogMessage(STR_LOG_MSG_FUNC21_CAM_CAPS_INIT_FAIL, LOG_SVRTY_ERR);
        kill(getpid(), SIGTERM);
        pthread_exit(NULL);
    }

    /* Build video streaming pipeline and set current video coding format */
    built = FALSE;
    for(i = 0;(i<NUM_SUP_VID_COD_FMT) && (!built);++i) {

        if(cameraCapabilities[i].supported) {

            if(0 == pipeBuilder(&pipeline, camDevPath, (VideoCodingFormat_T)(i), cameraCapabilities)) {
                currentCodingFormat = (VideoCodingFormat_T)(i);
                built = TRUE;
            }
        }
    }

    /* Check if pipeline was built */
    if(!built) {

        createLogMessage(STR_LOG_MSG_FUNC21_PIPE_BUILD_FAIL, LOG_SVRTY_ERR);
        kill(getpid(), SIGTERM);
        pthread_exit(NULL);
    }

    /* Register pipeline callback functions for error detection */
    if(registerCallbackFunctions(pipeline)) {

        createLogMessage(STR_LOG_MSG_FUNC21_REG_CBS_FAIL, LOG_SVRTY_ERR);

        gst_object_unref(pipeline);
        kill(getpid(), SIGTERM);
        pthread_exit(NULL);
    }

    /* Start main loop thread for pipeline event management */
    errorCode = pthread_create(&threadStreamMainLoop, NULL, threadFuncStreamMainLoop, NULL);
    if(0 != errorCode) {

        createLogMessage(STR_LOG_MSG_FUNC21_THRD_START_FAIL, LOG_SVRTY_ERR);

        gst_object_unref(pipeline);
        kill(getpid(), SIGTERM);
        pthread_exit(NULL);
    }

    initStreamController(streamController);

    while(1) {

        if(removeModuleMessage(&streamMsgq, &message, MOD_MSGQ_BLOCK)) {

            createLogMessage(STR_LOG_MSG_FUNC21_MSG_RMV_FAIL, LOG_SVRTY_WRN);
        }
        else {

            switch(message->code) {

                case MOD_MSG_CODE_STREAM_START:

                    event = STREAM_EVENT_STREAM_START;
                    updateRequired = SM_UPDATE_REQUIRED;
                    break;

                case MOD_MSG_CODE_STREAM_STOP:

                    event = STREAM_EVENT_STREAM_STOP;
                    updateRequired = SM_UPDATE_REQUIRED;
                    break;

                case MOD_MSG_CODE_STREAM_REQ:

                    event = STREAM_EVENT_STREAM_REQ;
                    updateRequired = SM_UPDATE_REQUIRED;
                    break;

                case MOD_MSG_CODE_STREAM_ERROR:

                    event = STREAM_EVENT_PIPE_ERROR;
                    updateRequired = SM_UPDATE_REQUIRED;
                    break;
            
                default:

                    createLogMessage(STR_LOG_MSG_FUNC21_CODE_INVAL, LOG_SVRTY_WRN);
                    updateRequired = SM_UPDATE_NOT_REQUIRED;
                    break;
            }

            if(updateRequired) {

                streamController[state][event].eventHandler(&message, &pipeline);
                state = streamController[state][event].nextState;
                updateRequired = SM_UPDATE_NOT_REQUIRED;
            }
        }
    }

    return NULL;
}

static void* threadFuncStreamMainLoop(void *arg) {

    GMainLoop *loop = NULL;

    loop = g_main_loop_new(NULL, FALSE);
    if(NULL != loop) {

        g_main_loop_run(loop);

        /* 
         * Nothing to do. Let the stream control
         * thread deal with the issues.
         */

        /* Clean up resources */
        g_main_loop_quit(loop);
        g_main_loop_unref(loop);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC35_LOOP_CREAT_FAIL, LOG_SVRTY_ERR);
    }

    return NULL;
}

static void initStreamController(StateContext_T controller[NUM_STREAM_STATE_NUM][NUM_STREAM_EVENT_NUM]) {

    controller[STREAM_STATE_STANDBY][STREAM_EVENT_STREAM_REQ]   = (StateContext_T) {.nextState = STREAM_STATE_STANDBY, .eventHandler = streamRequestHandler};
    controller[STREAM_STATE_STANDBY][STREAM_EVENT_STREAM_START] = (StateContext_T) {.nextState = STREAM_STATE_PLAYING, .eventHandler = streamStartHandler};
    controller[STREAM_STATE_STANDBY][STREAM_EVENT_STREAM_STOP]  = (StateContext_T) {.nextState = STREAM_STATE_STANDBY, .eventHandler = emptyHandler};
    controller[STREAM_STATE_STANDBY][STREAM_EVENT_PIPE_ERROR]   = (StateContext_T) {.nextState = STREAM_STATE_STANDBY, .eventHandler = streamErrorHandler};
    controller[STREAM_STATE_PLAYING][STREAM_EVENT_STREAM_REQ]   = (StateContext_T) {.nextState = STREAM_STATE_PLAYING, .eventHandler = emptyHandler};
    controller[STREAM_STATE_PLAYING][STREAM_EVENT_STREAM_START] = (StateContext_T) {.nextState = STREAM_STATE_PLAYING, .eventHandler = emptyHandler};
    controller[STREAM_STATE_PLAYING][STREAM_EVENT_STREAM_STOP]  = (StateContext_T) {.nextState = STREAM_STATE_STANDBY, .eventHandler = streamStopHandler};
    controller[STREAM_STATE_PLAYING][STREAM_EVENT_PIPE_ERROR]   = (StateContext_T) {.nextState = STREAM_STATE_STANDBY, .eventHandler = streamErrorHandler};
}

static void* emptyHandler(ModuleMessage_T* *message, GstElement* *pipeline) {

    if(NULL != message) {

        free(*message);
        *message = NULL;
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC36_ARG_INVAL, LOG_SVRTY_ERR);
    }

    return NULL;
}

static void* streamRequestHandler(ModuleMessage_T* *message, GstElement* *pipeline) {

    ModuleMessage_T *formatMessage = NULL;
    GstElement *networkSink = NULL;

    if((NULL != message) && (NULL != pipeline)) {

        /* Update video stream target port */
        networkSink = gst_bin_get_by_name(GST_BIN(*pipeline), STR_PIPE_ELEM_NAME_NETSINK);
        if(NULL != networkSink) {

            g_object_set(networkSink, "port", (gint)((*message)->data.videoStreamPort), NULL);
        }
        else {

            createLogMessage(STR_LOG_MSG_FUNC37_PORT_SET_FAIL, LOG_SVRTY_ERR);
        }

        free(*message);
        *message = NULL;

        /* Inform video stream target about the video coding type */
        formatMessage = (ModuleMessage_T*)calloc(1, sizeof(ModuleMessage_T));
        if(NULL != formatMessage) {

            formatMessage->address = MOD_NAME_GCCOMMON;
            formatMessage->code = MOD_MSG_CODE_STREAM_TYPE;
            formatMessage->data.codingFormat = currentCodingFormat;

            insertModuleMessage(&networkMsgq, formatMessage, MOD_MSGQ_BLOCK);
            formatMessage = NULL;
        }
        else {

            createLogMessage(STR_LOG_MSG_FUNC37_MSG_ALLOC_FAIL, LOG_SVRTY_ERR);
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC37_ARG_INVAL, LOG_SVRTY_ERR);
    }

    return NULL;
}

static void* streamStopHandler(ModuleMessage_T* *message, GstElement* *pipeline) {

    GstStateChangeReturn ret;

    if((NULL != message) && (NULL != pipeline)) {

        free(*message);
        *message = NULL;

        /* Set pipeline to its initial state */
        ret = gst_element_set_state(*pipeline, PIPE_INITIAL_STATE);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC38_PIPE_SET_INIT_FAIL, LOG_SVRTY_ERR);
            createLogMessage(STR_LOG_MSG_FUNC38_SM_STATE_INCON, LOG_SVRTY_INF);
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC38_ARG_INVAL, LOG_SVRTY_ERR);
    }

    return NULL;
}

static void* streamStartHandler(ModuleMessage_T* *message, GstElement* *pipeline) {

    GstStateChangeReturn ret;

    if((NULL != message) && (NULL != pipeline)) {

        free(*message);
        *message = NULL;

        /* Set pipeline to playing state */
        ret = gst_element_set_state(*pipeline, GST_STATE_PLAYING);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC39_PIPE_SET_PLAY_FAIL, LOG_SVRTY_ERR);
            createLogMessage(STR_LOG_MSG_FUNC39_SM_STATE_INCON, LOG_SVRTY_INF);
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC39_ARG_INVAL, LOG_SVRTY_ERR);
    }

    return NULL;
}

static void* streamErrorHandler(ModuleMessage_T* *message, GstElement* *pipeline) {

    GstStateChangeReturn ret;

    if((NULL != message) && (NULL != pipeline)) {

        /* Notify ground control by forwarding the message */
        (*message)->address = MOD_NAME_GCCOMMON;
        insertModuleMessage(&networkMsgq, *message, MOD_MSGQ_BLOCK);
        *message = NULL;

        /* Set pipeline to NULL state */
        ret = gst_element_set_state(*pipeline, GST_STATE_NULL);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC40_PIPE_SET_NULL_FAIL, LOG_SVRTY_ERR);
            createLogMessage(STR_LOG_MSG_FUNC40_SM_STATE_INCON, LOG_SVRTY_INF);
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC40_ARG_INVAL, LOG_SVRTY_ERR);
    }

    return NULL;
}

static int initCameraCapabilities(const char *camDevPath, VideoCodingFormatContext_T *initCtx) {

    int retval = 0;
    int terminate = FALSE;
    GError *error = NULL;
    gchar *debugInfo = NULL;
    GstElement *pipeline = NULL, *videoSource = NULL;
    GstBus *bus = NULL;
    GstMessage *message = NULL;
    GstState newState;
    GstStateChangeReturn ret;

    if((NULL != camDevPath) && (NULL != initCtx)) {

        /* Initialize capabilities array to zero */
        memset(initCtx->capsArray, 0, sizeof(VideoCodingFormatCaps_T)*initCtx->size);

        /* Instantiate pipeline and video source element */
        videoSource = gst_element_factory_make("v4l2src", "Video_Source");
        pipeline = gst_pipeline_new("Camera_Pipeline");

        if(!pipeline || !videoSource) {

            createLogMessage(STR_LOG_MSG_FUNC22_CREAT_ELEM_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }

        /* Set camera device path */
        g_object_set(videoSource, "device", camDevPath, NULL);

        /* Build the pipeline and set state to PAUSED */
        gst_bin_add(GST_BIN(pipeline), videoSource);
        ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC22_PIPE_STATE_SET_FAIL, LOG_SVRTY_ERR);
            gst_object_unref(pipeline);

            retval = -1;
            return retval;
        }

        /* Get pipeline bus and wait for state change or error message */
        bus = gst_element_get_bus(pipeline);
        do {

            message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED);

            /* Parse message */
            if(NULL != message) {

                switch(GST_MESSAGE_TYPE(message)) {

                    case GST_MESSAGE_ERROR:

                        /* Error occured in the media pipeline */
                        gst_message_parse_error(message, &error, &debugInfo);
                        #ifdef CC_DEBUG_MODE
                        fprintf(stdout, STR_LOG_MSG_FUNC22_PIPE_ELEM_ERROR_MSG, GST_OBJECT_NAME(message->src), error->message);
                        fprintf(stdout, STR_LOG_MSG_FUNC22_PIPE_ELEM_ERROR_DBG, (debugInfo ? debugInfo : "none"));
                        fflush(stdout);
                        #endif
                        syslog(LOG_DAEMON | LOG_ERR, STR_LOG_MSG_FUNC22_PIPE_ELEM_ERROR_MSG, GST_OBJECT_NAME(message->src), error->message);
                        syslog(LOG_DAEMON | LOG_ERR, STR_LOG_MSG_FUNC22_PIPE_ELEM_ERROR_DBG, (debugInfo ? debugInfo : "none"));

                        g_clear_error(&error);
                        g_free(debugInfo);

                        terminate = TRUE;
                        retval = -1;
                        break;

                    case GST_MESSAGE_STATE_CHANGED:

                        /* State-change occured in the media pipeline (NULL -> PAUSED) */
                        if(GST_MESSAGE_SRC(message) == GST_OBJECT(pipeline)) {
                            
                            gst_message_parse_state_changed(message, NULL, &newState, NULL);
                            if(GST_STATE_PAUSED == newState) {

                                if(getCameraCapabilities(videoSource, initCtx)) {

                                    createLogMessage(STR_LOG_MSG_FUNC22_CAM_CAPS_GET_FAIL, LOG_SVRTY_ERR);
                                    retval = -1;
                                }

                                terminate = TRUE;
                            }
                        }
                        break;

                    default:

                        /* Unexpected message received */
                        createLogMessage(STR_LOG_MSG_FUNC22_MSG_UNEXP, LOG_SVRTY_ERR);

                        terminate = TRUE;
                        retval = -1;
                        break;
                }

                gst_message_unref(message);
            }
        } while(!terminate);

        /* Stop pipeline and free resourcces */
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC22_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

static int pipeBuilder(GstElement* *pipeline, const char *camDevPath, const VideoCodingFormat_T codingFormat, const VideoCodingFormatCaps_T caps[NUM_SUP_VID_COD_FMT]) {

    int retval = 0;
    char mediaType[32] = {0};

    GstStateChangeReturn ret;
    GstCaps *capsConfig = NULL;
    GstElement *videoSource = NULL;
    GstElement *videoConverter = NULL;
    GstElement *capsfilter = NULL;
    GstElement *encoder = NULL;
    GstElement *payloader = NULL;
    GstElement *networkSink = NULL;

    if((NULL != pipeline) && (NULL != camDevPath) && (NUM_SUP_VID_COD_FMT > codingFormat)) {

        /* Instantiate pipeline and its elements */

        videoSource = gst_element_factory_make("v4l2src", STR_PIPE_ELEM_NAME_VIDSRC);
        if(CAM_FMT_RAW == codingFormat) {

            /* Use OpenMax H.264 for RAW camera output */
            videoConverter = gst_element_factory_make("autovideoconvert", STR_PIPE_ELEM_NAME_VIDCONV);
            capsfilter = gst_element_factory_make("capsfilter", STR_PIPE_ELEM_NAME_CAPSFLTR);
            capsConfig = gst_caps_new_simple(
                "video/x-raw",
                "width", G_TYPE_INT, caps[CAM_FMT_RAW].width,
                "height", G_TYPE_INT, caps[CAM_FMT_RAW].height,
                "framerate", GST_TYPE_FRACTION, caps[CAM_FMT_RAW].framerateNumerator, caps[CAM_FMT_RAW].framerateDenominator,
                NULL
            );
            g_object_set(capsfilter, "caps", capsConfig, NULL);
            gst_caps_unref(capsConfig);
            encoder = gst_element_factory_make("omxh264enc", STR_PIPE_ELEM_NAME_ENCODER);
            payloader = gst_element_factory_make("rtph264pay", STR_PIPE_ELEM_NAME_PAYLDR);
        }
        else {

            videoCodingFormatToString(codingFormat, mediaType, sizeof(mediaType));
            capsfilter = gst_element_factory_make("capsfilter", STR_PIPE_ELEM_NAME_CAPSFLTR);
            capsConfig = gst_caps_new_simple(
                mediaType,
                "width", G_TYPE_INT, caps[codingFormat].width,
                "height", G_TYPE_INT, caps[codingFormat].height,
                "framerate", GST_TYPE_FRACTION, caps[codingFormat].framerateNumerator, caps[codingFormat].framerateDenominator,
                NULL
            );
            g_object_set(capsfilter, "caps", capsConfig, NULL);
            gst_caps_unref(capsConfig);

            switch(codingFormat) {

                case CAM_FMT_H265:
                    payloader = gst_element_factory_make("rtph265pay", STR_PIPE_ELEM_NAME_PAYLDR);
                    break;
                
                case CAM_FMT_H264:
                    payloader = gst_element_factory_make("rtph264pay", STR_PIPE_ELEM_NAME_PAYLDR);
                    break;

                case CAM_FMT_VP8:
                    payloader = gst_element_factory_make("rtpvp8pay", STR_PIPE_ELEM_NAME_PAYLDR);
                    break;

                case CAM_FMT_VP9:
                    payloader = gst_element_factory_make("rtpvp9pay", STR_PIPE_ELEM_NAME_PAYLDR);
                    break;

                case CAM_FMT_JPEG:
                    payloader = gst_element_factory_make("rtpjpegpay", STR_PIPE_ELEM_NAME_PAYLDR);
                    break;

                case CAM_FMT_H263:
                    payloader = gst_element_factory_make("rtph263pay", STR_PIPE_ELEM_NAME_PAYLDR);
                    break;

                default:
                    createLogMessage(STR_LOG_MSG_FUNC30_CODING_FMT_INVAL, LOG_SVRTY_ERR);
                    gst_object_unref(videoSource);
                    retval = -1;
                    return retval;
            }
        }
        networkSink = gst_element_factory_make("udpsink", STR_PIPE_ELEM_NAME_NETSINK);
        *pipeline = gst_pipeline_new("Video_Streaming_Pipeline");

        if(CAM_FMT_RAW == codingFormat) {

            if (!(*pipeline) || !videoSource || !videoConverter || !capsfilter || !encoder || !payloader || !networkSink) {

                createLogMessage(STR_LOG_MSG_FUNC30_CREAT_ELEM_FAIL , LOG_SVRTY_ERR);

                *pipeline = NULL;
                retval = -1;
                return retval;
            }
        }
        else {

            if (!(*pipeline) || !videoSource || !capsfilter || !payloader || !networkSink) {

                createLogMessage(STR_LOG_MSG_FUNC30_CREAT_ELEM_FAIL , LOG_SVRTY_ERR);

                *pipeline = NULL;
                retval = -1;
                return retval;
            }
        }

        /* Set pipeline common elements' properties */
        g_object_set(videoSource, "device", camDevPath, NULL);
        g_object_set(payloader, "mtu", NUM_UDP_MTU, NULL);
        g_object_set(
            
            networkSink,
            "host", STR_STREAM_DEST_ADDR,
            "port", NUM_STREAM_DEST_PORT,
            "sync", FALSE,
            "async", FALSE, NULL
        );

        /* Build the pipeline */
        if(CAM_FMT_RAW == codingFormat) {

            gst_bin_add_many(GST_BIN(*pipeline), videoSource, videoConverter, capsfilter, encoder, payloader, networkSink, NULL);
            if(TRUE != gst_element_link_many(videoSource, videoConverter, capsfilter, encoder, payloader, networkSink, NULL)) {

                createLogMessage(STR_LOG_MSG_FUNC30_PIPE_LINK_FAIL, LOG_SVRTY_ERR);

                gst_object_unref(*pipeline);
                *pipeline = NULL;
                retval = -1;
                return retval;
            }
        }
        else {

            gst_bin_add_many(GST_BIN(*pipeline), videoSource, capsfilter, payloader, networkSink, NULL);
            if(TRUE != gst_element_link_many(videoSource, capsfilter, payloader, networkSink, NULL)) {

                createLogMessage(STR_LOG_MSG_FUNC30_PIPE_LINK_FAIL, LOG_SVRTY_ERR);

                gst_object_unref(*pipeline);
                *pipeline = NULL;
                retval = -1;
                return retval;
            }
        }

        /* Set pipeline to its initial state */
        ret = gst_element_set_state(*pipeline, PIPE_INITIAL_STATE);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC30_PIPE_SET_INIT_FAIL, LOG_SVRTY_ERR);

            gst_object_unref(*pipeline);
            *pipeline = NULL;
            retval = -1;
            return retval;
        }

        /* Log debug info */
        #ifdef CC_DEBUG_MODE
        fprintf(stdout, STR_LOG_MSG_FUNC30_PIPE_TYPE_INFO, mediaType);
        fflush(stdout);
        #endif
        syslog(LOG_DAEMON | LOG_INFO, STR_LOG_MSG_FUNC30_PIPE_TYPE_INFO, mediaType);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC30_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

static void pipelineErrorCallback(GstBus *bus, GstMessage *message, gpointer data) {

    GError *error = NULL;
    gchar *debugInfo = NULL;
    ModuleMessage_T *moduleMessage = NULL;

    if(NULL != message) {

        gst_message_parse_error(message, &error, &debugInfo);

        #ifdef CC_DEBUG_MODE
        fprintf(stdout, STR_LOG_MSG_FUNC31_PIPE_ELEM_ERROR_MSG, GST_OBJECT_NAME(message->src), error->message);
        fprintf(stdout, STR_LOG_MSG_FUNC31_PIPE_ELEM_ERROR_DBG, (debugInfo ? debugInfo : "none"));
        fflush(stdout);
        #endif
        syslog(LOG_DAEMON | LOG_INFO, STR_LOG_MSG_FUNC31_PIPE_ELEM_ERROR_MSG, GST_OBJECT_NAME(message->src), error->message);
        syslog(LOG_DAEMON | LOG_INFO, STR_LOG_MSG_FUNC31_PIPE_ELEM_ERROR_DBG, (debugInfo ? debugInfo : "none"));

        moduleMessage = (ModuleMessage_T*)calloc(1, sizeof(ModuleMessage_T));
        if(NULL != moduleMessage) {

            moduleMessage->address = MOD_NAME_STREAM;
            moduleMessage->code = MOD_MSG_CODE_STREAM_ERROR;
            insertModuleMessage(&streamMsgq, moduleMessage, MOD_MSGQ_BLOCK);
            moduleMessage = NULL;
        }
        else {

            createLogMessage(STR_LOG_MSG_FUNC31_MSG_ALLOC_FAIL, LOG_SVRTY_ERR);
        }

        g_error_free(error);
        g_free(debugInfo);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC31_ARG_INVAL, LOG_SVRTY_ERR);
    }
}

static void pipelineEosCallback(GstBus *bus, GstMessage *message, gpointer data) {

    ModuleMessage_T *moduleMessage = NULL;

    createLogMessage(STR_LOG_MSG_FUNC32_PIPE_EOS, LOG_SVRTY_INF);

    /* We should not reach EOS so we handle it as an error */

    moduleMessage = (ModuleMessage_T*)calloc(1, sizeof(ModuleMessage_T));
    if(NULL != moduleMessage) {

        moduleMessage->address = MOD_NAME_STREAM;
        moduleMessage->code = MOD_MSG_CODE_STREAM_ERROR;
        insertModuleMessage(&streamMsgq, moduleMessage, MOD_MSGQ_BLOCK);
        moduleMessage = NULL;
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC32_MSG_ALLOC_FAIL, LOG_SVRTY_ERR);
    }
}

static void pipelineStatechangedCallback(GstBus *bus, GstMessage *message, gpointer data) {

    GstElement *pipeline = (GstElement*)data;
    GstState oldState, newState, pendingState;

    if((NULL != pipeline) && (NULL != message)) {

        /* Filter message source on pipeline */
        if(GST_MESSAGE_SRC(message) == GST_OBJECT(pipeline)) {

            gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
            #ifdef CC_DEBUG_MODE
            fprintf(stdout, STR_LOG_MSG_FUNC33_PIPE_STATE_CHANGE,
                gst_element_state_get_name(oldState), gst_element_state_get_name(newState));
            fflush(stdout);
            #endif
            syslog(LOG_DAEMON | LOG_INFO, STR_LOG_MSG_FUNC33_PIPE_STATE_CHANGE,
                gst_element_state_get_name(oldState), gst_element_state_get_name(newState));
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC33_ARG_INVAL, LOG_SVRTY_ERR);
    }
}

static int registerCallbackFunctions(GstElement *pipeline) {

    int retval = 0;
    GstBus *bus = NULL;

    if(NULL != pipeline) {

        bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

        gst_bus_add_signal_watch(bus);
        g_signal_connect(bus, "message::error", G_CALLBACK(pipelineErrorCallback), NULL);
        g_signal_connect(bus, "message::eos", G_CALLBACK(pipelineEosCallback), NULL);
        g_signal_connect(bus, "message::state-changed", G_CALLBACK(pipelineStatechangedCallback), pipeline);

        gst_object_unref(bus);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC34_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}
