/**
 * @file        camera_utils.h
 * @author      Adam Csizy
 * @date        2021-03-19
 * @version     v1.1.0
 * 
 * @brief       Camera utilities
 */

#pragma once


#include <gst/gst.h>


/* Camera related public macro definitions */

#define NUM_SUP_VID_COD_FMT   7U    /**< Number of supported video coding formats (see VideoCodingFormat_T). */


/* Camera related public type definitions */

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

/**
 * @brief       Video coding format capabilities.
 *          
 * @details     A structure for storing capabilities for any
 *              video coding format supported by this software.
 *              Each member field describes a capability. Note
 *              that the structure functions as a union of
 *              capabilities and some of them might not be used
 *              for a given video coding format.           
 */
typedef struct VideoCodingFormatCaps {

  int supported;              /**< Flag whether the format is supported */
  int width;                  /**< Frame width */
  int height;                 /**< Frame height */
  int framerateNumerator;     /**< Framerate numerator */
  int framerateDenominator;   /**< Framerate denominator */

} VideoCodingFormatCaps_T;

/**
 * @brief       Context for video coding formats.
 * 
 * @details     Used as a wrapper to encapsulate the array
 *              of video coding format capabilities and its
 *              size.
 */
typedef struct VideoCodingFormatContext {

  VideoCodingFormatCaps_T *capsArray;   /**< Array of video coding format capabilities */
  size_t size;                          /**< Size of the array */

} VideoCodingFormatContext_T;


/* Camera related public function declarations */

/**
 * @brief       Converts video coding format to string.
 * 
 * @details     Converts the given VideoCodingFormat_T
 *              representation of a video coding format
 *              into a string representation.
 * 
 * @note        None
 * 
 * @param[in]   format VideoCodingFormat_T representation.
 * @param[out]  string String representation of video coding format.
 * @param[in]   size Size of the string.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int videoCodingFormatToString(const VideoCodingFormat_T format, char string[], const size_t size);

/**
 * @brief       Searches and validates camera device.
 * 
 * @details     Searches for camera devices under '/dev' directory
 *              and returns with the path of the first compatible
 *              device entry. Validation includes V4L2 interface
 *              compatibility and Video Capture capability.
 * 
 * @note        None
 * 
 * @param[out]  cameraDevicePath Path string.
 * @param[in]   size Size of path string.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int getCameraDevicePath(char cameraDevicePath[], const size_t size);

/**
 * @brief       Retrieves capabilities for the given camera device.
 * 
 * @details     Retrieves capabilities for the given video camera
 *              device represented as a v4l2src pipeline element.
 *              This function tries to retrieve the best possible
 *              capability configuration for each video encoding
 *              format. The retrieved capabilities for each format
 *              are being stored in an array encapsulated in the
 *              user data context (cameraCapsContext).
 * 
 * @note        This function should be called when the given
 *              camera device element is in READY or higher state.
 *              It is necessary because the v4l2src element needs to
 *              query the underlying hardware to refine and distinct
 *              its capabilities from the pad template's capabilities.
 * 
 * @param[in]   v4l2srcElement Pointer to a v4l2src pipeline element representing the corresponding camera device.
 * @param[in,out]   ctx User data context.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int getCameraCapabilities(GstElement *v4l2srcElement, VideoCodingFormatContext_T *ctx);