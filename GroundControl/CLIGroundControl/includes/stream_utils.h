/**
 * @file        stream_utils.h
 * @author      Adam Csizy
 * @date        2021-04-14
 * @version     v1.1.0
 * 
 * @brief       Streaming utilities
 */

#pragma once


#include <gst/gst.h>


/* Streaming related public function declarations */

/**
 * @brief       Initialize streaming services.
 * 
 * @details     Initializes GStreamer core and its plugins.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int initStreamServices(void);

/**
 * @brief       Stop video stream.
 * 
 * @details     Stops the RTP video display on the
 *              given pipeline by setting the pipeline's
 *              state to its initial state.
 * 
 * @note        GStreamer core and plugins must be initialized
 *              before invoking this function.
 * 
 *              After invoking this function the drone must be
 *              notified to stop transmitting RTP video stream.
 * 
 * @param [in,out]  pipeline GStreamer pipeline to be stopped. 
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int stopStream(GstElement* *pipeline);

/**
 * @brief       Request video stream.
 * 
 * @details     Requests RTP video stream from the drone and
 *              starts the ground control video display pipeline.
 *              On request the video coding format is negotiated
 *              and the GStreamer pipeline is build accordingly.
 *              The pipeline is being rebuilt only if it is not
 *              existing yet or the coding format does not match.
 * 
 * @note        GStreamer core and plugins must be initialized
 *              before invoking this function.
 * 
 * @param [in]  socketFd File descriptor of service socket.
 * @param [in,out]  pipeline GStreamer pipeline to be stopped. 
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int requestStream(const int socketFd, GstElement* *pipeline);