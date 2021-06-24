/**
 * @file        stream_utils.c
 * @author      Adam Csizy
 * @date        2021-04-14
 * @version     v1.1.0
 * 
 * @brief       Streaming utilities
 */


#include <gst/gst.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "com_utils.h"
#include "log_utils.h"
#include "stream_utils.h"


/* Streaming related macro definitions */

#define NUM_STREAM_SRC_PORT         5000 /**< Default service port of RTP stream source */
//#define NUM_STREAM_PORT_DRONE       5000 /**< Port to which the drone streams the RTP video. (LAN) */
#define NUM_STREAM_PORT_DRONE       17000 /**< Port to which the drone streams the RTP video. (WAN) */
#define PIPE_INITIAL_STATE          GST_STATE_READY /**< Initial state of the video display pipeline */
#define NUM_MSG_HEADER_SIZE         2U          /**< Size of message header array in MessageHeaderField_T */
#define IDX_MSG_HEADER_MODULE       0U        /**< Index of module name in message header array */
#define IDX_MSG_HEADER_CODE         1U          /**< Index of module message code in message header array */
#define NUM_UDP_MTU                 64000 /**< MTU for UDP packets in bytes. Theoretical ceiling is 64kB but GStreamer payloaders might not support such a high value.  */

#define MessageHeaderField_T uint32_t /**< Type of the fields in the header of network messages */


/* Streaming related static global variable declarations */

static pthread_t threadStreamMainLoop; /**< Thread object for handling main loop context of the video stream */
static GMainLoop *loop = NULL;  /* Main loop context */


/* Streaming related static function declarations */

/**
 * @brief       Start routine of stream main loop thread.
 * 
 * @details     Initializes and starts a GMainLoop object
 *              using the default context. In terms of the
 *              video display application the main loop is
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
 * @brief       Build media pipeline.
 * 
 * @details     Builds a GStreamer media pipeline compatible with
 *              the given video coding format as the network
 *              source's output. The video stream is displayed
 *              in a separate window native to the underlying OS.
 * 
 * @note        GStreamer core and plugins must be initialized
 *              using 'gst_init()' before invoking this function.
 *
 * @param[in,out]   pipeline Pointer to a pipeline to be built.
 * @param[in]   codingFormat Video coding format.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int pipeBuilder(GstElement* *pipeline, const VideoCodingFormat_T codingFormat);

/**
 * @brief       Pipeline error signal callback.
 * 
 * @details     Callback function for handling pipeline error
 *              signals. On pipeline error a log record is created,
 *              the user notified and the video display pipeline
 *              stopped (NULL state).
 *
 * @param[in,out]   bus Pipeline's bus.
 * @param[in]   message Pipeline error message.
 * @param[in]   data Custom data: pointer to the video display pipeline.
 */
static void pipelineErrorCallback(GstBus *bus, GstMessage *message, gpointer data);


/* Streaming related function definitions */

int stopStream(GstElement* *pipeline){

    int retval = 0;
    GstStateChangeReturn ret;

    if(NULL != pipeline) {

        if(NULL != *pipeline) {

            /* Set pipeline to its initial state */
            ret = gst_element_set_state(*pipeline, PIPE_INITIAL_STATE);
            if(GST_STATE_CHANGE_FAILURE == ret) {

                createLogMessage(STR_LOG_MSG_FUNC10_PIPE_SET_INIT_FAIL, LOG_SVRTY_ERR);
                retval = -1;
            }
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC10_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

int requestStream(const int socketFd, GstElement* *pipeline) {

    int retval = 0;
    int length;
    uint32_t codingFormat = 0U;
    VideoStreamPort_T streamPort = 0U;
    MessageHeaderField_T messageHeader[NUM_MSG_HEADER_SIZE] = {0};
    GstStateChangeReturn ret;
    GstClockTime stateChangeTimeout = 5000000000; // 5 sec in nanosecs

    if((0 > socketFd) || (NULL == pipeline)) {

        createLogMessage(STR_LOG_MSG_FUNC12_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }
    else {

        /* Request video stream on specified port */
        messageHeader[IDX_MSG_HEADER_MODULE] = MOD_NAME_STREAM;
        messageHeader[IDX_MSG_HEADER_CODE] = MOD_MSG_CODE_STREAM_REQ;
        streamPort = NUM_STREAM_PORT_DRONE;

        length = send(socketFd, messageHeader, sizeof(messageHeader), MSG_NOSIGNAL);
        if(sizeof(messageHeader) > length) {
            
            if(0 > length) {

                perror("send");
                fflush(stderr);
            }
            createLogMessage(STR_LOG_MSG_FUNC12_MSG_REQ_SEND_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }

        length = send(socketFd, &streamPort, sizeof(streamPort), MSG_NOSIGNAL);
        if(sizeof(streamPort) > length) {
            
            if(0 > length) {

                perror("send");
                fflush(stderr);
            }
            createLogMessage(STR_LOG_MSG_FUNC12_MSG_PORT_SEND_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }

        /* Receive video coding format */
        length = recvTimeout(socketFd, messageHeader, sizeof(messageHeader), MSG_WAITALL, 2, 0);
        if(sizeof(messageHeader) > length) {
            
            if(0 > length) {

                perror("recv");
                fflush(stderr);
            }
            createLogMessage(STR_LOG_MSG_FUNC12_MSG_TYP_RECV_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }

        /* Validate message header */
        if(MOD_MSG_CODE_STREAM_TYPE != messageHeader[IDX_MSG_HEADER_CODE]) {

            createLogMessage(STR_LOG_MSG_FUNC12_MSG_TYP_INVAL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }

        length = recvTimeout(socketFd, &codingFormat, sizeof(codingFormat), MSG_WAITALL, 2, 0);
        if(sizeof(codingFormat) > length) {

            if(0 > length) {

                perror("recv");
                fflush(stderr);
            }
            createLogMessage(STR_LOG_MSG_FUNC12_MSG_FMT_RECV_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }

        /* Build pipeline if necessary */
        if(NULL == *pipeline) {

            if(pipeBuilder(pipeline, (VideoCodingFormat_T)(codingFormat))) {

                createLogMessage(STR_LOG_MSG_FUNC12_PIPE_BUILD_FAIL, LOG_SVRTY_ERR);
                retval = -1;
                return retval;
            }
        }

        /* Set state to playing */
        ret = gst_element_set_state(*pipeline, GST_STATE_PLAYING);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC12_PIPE_SET_PLAY_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }
        else if(GST_STATE_CHANGE_ASYNC == ret) {

            /* Wait for asynchronous state change completion */
            gst_element_get_state(*pipeline, NULL, NULL, stateChangeTimeout);
        }

        /* Send play message */
        messageHeader[IDX_MSG_HEADER_MODULE] = MOD_NAME_STREAM;
        messageHeader[IDX_MSG_HEADER_CODE] = MOD_MSG_CODE_STREAM_START;
        length = send(socketFd, messageHeader, sizeof(messageHeader), MSG_NOSIGNAL);
        if(sizeof(messageHeader) > length) {

            if(0 > length) {

                perror("send");
                fflush(stderr);
            }
            createLogMessage(STR_LOG_MSG_FUNC12_MSG_START_SEND_FAIL, LOG_SVRTY_ERR);
            retval = -1;
            return retval;
        }
    }

    return retval;
}

static int pipeBuilder(GstElement* *pipeline, const VideoCodingFormat_T codingFormat) {

    int retval = 0;

    GstStateChangeReturn ret;
    GstCaps *caps = NULL;
    GstElement *networkSource = NULL;
    GstElement *capsfilter = NULL;
    GstElement *depayloader = NULL;
    GstElement *decoder = NULL;
    GstElement *videoConverter = NULL;
    GstElement *videoRescaler = NULL;
    GstElement *videoSink = NULL;
    GstBus *bus = NULL;

    if((NULL != pipeline) && (NUM_SUP_VID_COD_FMT > codingFormat)) {

        /* Instantiate pipeline and its elements */
        networkSource = gst_element_factory_make("udpsrc", "UDP_Network_Source");
        capsfilter = gst_element_factory_make("capsfilter", "Capabilities_Filter");

        switch(codingFormat) {

            case CAM_FMT_H265:

                depayloader = gst_element_factory_make("rtph265depay", "RTP_H265_Depayloader");
                decoder = gst_element_factory_make("avdec_h265", "H265_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "H265",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;
            
            case CAM_FMT_H264:

                depayloader = gst_element_factory_make("rtph264depay", "RTP_H264_Depayloader");
                decoder = gst_element_factory_make("avdec_h264", "H264_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "H264",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;

            case CAM_FMT_VP8:

                depayloader = gst_element_factory_make("rtpvp8depay", "RTP_VP8_Depayloader");
                decoder = gst_element_factory_make("vp8dec", "VP8_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "VP8",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;

            case CAM_FMT_VP9:

                depayloader = gst_element_factory_make("rtpvp9depay", "RTP_VP9_Depayloader");
                decoder = gst_element_factory_make("vp9dec", "VP9_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "VP9",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;

            case CAM_FMT_JPEG:

                depayloader = gst_element_factory_make("rtpjpegdepay", "RTP_JPEG_Depayloader");
                decoder = gst_element_factory_make("jpegdec", "JPEG_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "JPEG",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;

            case CAM_FMT_H263:

                depayloader = gst_element_factory_make("rtph263depay", "RTP_H263_Depayloader");
                decoder = gst_element_factory_make("avdec_h263", "H263_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "H263",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;

            case CAM_FMT_RAW:

                /* Use H.264 for RAW camera output */
                depayloader = gst_element_factory_make("rtph264depay", "RTP_H264_Depayloader");
                decoder = gst_element_factory_make("avdec_h264", "H264_Decoder");
                caps = gst_caps_new_simple(
                    "application/x-rtp", 
                    "clock-rate", G_TYPE_INT, 90000,
                    "media", G_TYPE_STRING, "video",
                    "encoding-name", G_TYPE_STRING, "H264",
                    NULL
                );
                g_object_set(capsfilter, "caps", caps, NULL);
                gst_caps_unref(caps);
                break;                    

            default:

                createLogMessage(STR_LOG_MSG_FUNC6_FMT_INVAL, LOG_SVRTY_ERR);
                gst_object_unref(networkSource);
                gst_object_unref(capsfilter);
                retval = -1;
                return retval;
        }

        videoConverter = gst_element_factory_make("videoconvert", "Video_Converter");
        videoRescaler = gst_element_factory_make("videoscale", "Video_Rescaler");
        videoSink = gst_element_factory_make("autovideosink", "Video_Sink");

        *pipeline = gst_pipeline_new("Video_Display_Pipeline");

        if (!(*pipeline) || !networkSource || !capsfilter || !depayloader ||
                !decoder || !videoConverter || ! videoRescaler || ! videoSink) {

            createLogMessage(STR_LOG_MSG_FUNC6_CREAT_ELEM_FAIL , LOG_SVRTY_ERR);

            *pipeline = NULL;
            retval = -1;
            return retval;
        }

        /* Set pipeline common elements' properties */
        g_object_set(
            
            networkSource,
            "port", NUM_STREAM_SRC_PORT,
            "reuse", TRUE,
            "mtu", NUM_UDP_MTU,
            NULL
        );
        g_object_set(videoSink, "sync", FALSE, NULL);

        /* Build the pipeline */
        gst_bin_add_many(GST_BIN(*pipeline), networkSource, capsfilter, depayloader, decoder,
                 videoConverter, videoRescaler, videoSink, NULL);
        if(TRUE != gst_element_link_many(networkSource, capsfilter, depayloader, decoder, 
                videoConverter, videoRescaler, videoSink, NULL)) {

            createLogMessage(STR_LOG_MSG_FUNC6_PIPE_LINK_FAIL, LOG_SVRTY_ERR);

            gst_object_unref(*pipeline);
            *pipeline = NULL;
            retval = -1;
            return retval;
        }

        /* Register callback functions (only error detection) */
        bus = gst_pipeline_get_bus(GST_PIPELINE(*pipeline));
        gst_bus_add_signal_watch(bus);
        g_signal_connect(bus, "message::error", G_CALLBACK (pipelineErrorCallback), *pipeline);
        gst_object_unref(bus);

        /* Start global main loop */
        
        // Note: Protect the creation of main loop on multithreaded applications

        if(pthread_create(&threadStreamMainLoop, NULL, threadFuncStreamMainLoop, &loop)) {
            createLogMessage(STR_LOG_MSG_FUNC6_MAIN_LOOP_START_FAIL, LOG_SVRTY_ERR);
        }

        /*
         * As for multithreaded applications the main loop is also
         * a shared global variable. The main loop is launched by
         * the first pipeline builder after constructing the pipeline.
         * Consecutive pipebuilders (threads) must ensure that the
         * mainloop is already running so it is not started multiple
         * times. For such scenarios a shared variable shoud be used
         * protected with mutex and indicatin the main loop status.
         * 
         * A much simpler alternative is to launch the main loop
         * on program initialization. Although adding watchers to
         * the default context with main loop running on needs to
         * be tested !
         */

        /* Set pipeline to its initial state */
        ret = gst_element_set_state(*pipeline, PIPE_INITIAL_STATE);
        if(GST_STATE_CHANGE_FAILURE == ret) {

            createLogMessage(STR_LOG_MSG_FUNC6_PIPE_SET_INIT_FAIL, LOG_SVRTY_ERR);

            gst_object_unref(*pipeline);
            *pipeline = NULL;
            retval = -1;
            return retval;
        }
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC6_ARG_INVAL, LOG_SVRTY_ERR);
        retval = -1;
    }

    return retval;
}

int initStreamServices(void) {

    int retval = 0;

    if(FALSE == gst_init_check(NULL, NULL, NULL)) {

        createLogMessage(STR_LOG_MSG_FUNC7_GST_INIT_FAIL, LOG_SVRTY_ERR);

        retval = -1;
        return retval;
    }

    return retval;
}

static void pipelineErrorCallback(GstBus *bus, GstMessage *message, gpointer data) {

    GstElement *pipeline = (GstElement*)data;

    gst_element_set_state(pipeline, GST_STATE_NULL);
    createLogMessage(STR_LOG_MSG_FUNC14_PIPE_ERROR, LOG_SVRTY_ERR);
    fprintf(stdout, "\nError detected in video display pipeline. Please issue the 'stop' command to reset the system.\n");
}

static void* threadFuncStreamMainLoop(void *arg) {

    GMainLoop* *loop = (GMainLoop**)arg;

    *loop = g_main_loop_new(NULL, FALSE);
    if(NULL != *loop) {

        g_main_loop_run(*loop);

        /* 
         * Nothing to do. Let the stream control
         * thread deal with the issues.
         */

        /* Clean up resources */
        g_main_loop_quit(*loop);
        g_main_loop_unref(*loop);
    }
    else {

        createLogMessage(STR_LOG_MSG_FUNC15_LOOP_CREAT_FAIL, LOG_SVRTY_ERR);
    }

    return NULL;
}