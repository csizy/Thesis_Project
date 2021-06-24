/**
 * @file        camera_utils.c
 * @author      Adam Csizy
 * @date        2021-03-19
 * @version     v1.1.0
 * 
 * @brief       Camera utilities
 */


#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <gst/gst.h>

#include "camera_utils.h"
#include "log_utils.h"


/* Camera related macro definitions */

#define STR_SIZE_DEV_PATH       16U                 /**< Size of string containing device path */
#define STR_HINT_CAM_DEV_NAME   "video"             /**< Hint for video device name */
#define STR_DEV_DIR_PATH        "/dev"              /**< Path to device directory */
#define STR_FORMAT_DEV_CAPS     "V4L2 Device Capabilities\n\nDriver name:\t%s\nDriver version:\t%d\nDevice name:\t%s\nBus info:\t%s\nCapabilities:\t%d\n"   /**< Device capabilities string format */
#define STR_FORMAT_FMTS_CAPS_TITLE "Supported camera formats and capabilities:\n\n" /**< Video coding formats and capabilities title string format */
#define STR_FORMAT_FMT_GENERAL  "\tFormat: %s\n"    /**< General video coding prefix string format*/
#define STR_FORMAT_CAPS_COMMON  "\tWidth: %d\n\tHeight: %d\n\tFramerate: %d/%d\n" /**< Common video coding capabilities string format */

#define STR_CAM_OUT_FMT_H265    "video/x-h265"      /**< Camera output video coding format: H.265 */
#define STR_CAM_OUT_FMT_H264    "video/x-h264"      /**< Camera output video coding format: H.264 */
#define STR_CAM_OUT_FMT_H263    "video/x-h263"      /**< Camera output video coding format: H.263 */
#define STR_CAM_OUT_FMT_JPEG    "image/jpeg"        /**< Camera output video coding format: JPEG */
#define STR_CAM_OUT_FMT_RAW     "video/x-raw"       /**< Camera output video coding format: RAW */
#define STR_CAM_OUT_FMT_VP8     "video/x-vp8"       /**< Camera output video coding format: VP8 */
#define STR_CAM_OUT_FMT_VP9     "video/x-vp9"       /**< Camera output video coding format: VP9 */
#define STR_CAM_OUT_FMT_UNK     "unknown"           /**< Camera output video coding format: unknown */

#define CAM_FMT_SUPPORTED       1                   /**< Supported camera output video coding format */
#define CAM_FMT_NOT_SUPPORTED   0                   /**< Unsupported camera output video coding format */


/* Camera related function definitions */

int videoCodingFormatToString(const VideoCodingFormat_T format, char string[], const size_t size) {

  int retval = 0;

  if((NULL != string) && (size)) {

    memset(string, 0, size);
    switch(format) {

      case CAM_FMT_H265:

        strncpy(string, STR_CAM_OUT_FMT_H265, (size-1));
        break;
      
      case CAM_FMT_H264:

        strncpy(string, STR_CAM_OUT_FMT_H264, (size-1));
        break;

      case CAM_FMT_VP8:

        strncpy(string, STR_CAM_OUT_FMT_VP8, (size-1));
        break;

      case CAM_FMT_VP9:

        strncpy(string, STR_CAM_OUT_FMT_VP9, (size-1));
        break;

      case CAM_FMT_JPEG:

        strncpy(string, STR_CAM_OUT_FMT_JPEG, (size-1));
        break;

      case CAM_FMT_H263:

        strncpy(string, STR_CAM_OUT_FMT_H263, (size-1));
        break;

      case CAM_FMT_RAW:

        strncpy(string, STR_CAM_OUT_FMT_RAW, (size-1));
        break;

      default:

        strncpy(string, STR_CAM_OUT_FMT_UNK, (size-1));
        break;
    }
  }
  else {

    createLogMessage(STR_LOG_MSG_FUNC41_ARG_INVAL, LOG_SVRTY_ERR);
    retval = -1;
  }

  return retval;
}

/**
 * @brief       Converts string to video coding format.
 * 
 * @details     Converts the given string representation of a
 *              video encoding format into a VideoCodingFormat_T
 *              representation. If encoding format cannot be
 *              identified an unknown format (CAM_FMT_UNK)
 *              is set.
 * 
 * @note        None
 * 
 * @param[in]   string String representation of the video coding format.
 * @param[out]  format Video coding format.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int stringToVideoCodingFormat(const char *string, VideoCodingFormat_T *format) {

  int retval = 0;

  if((NULL != string) && (NULL != format)) {

    /* Identify video output format */

    if(!strcmp(string, STR_CAM_OUT_FMT_RAW)) {

      *format = CAM_FMT_RAW;
    }
    else if(!strcmp(string, STR_CAM_OUT_FMT_JPEG)) {

      *format = CAM_FMT_JPEG;
    }
    else if(!strcmp(string, STR_CAM_OUT_FMT_H264)) {

      *format = CAM_FMT_H264;
    }
    else if(!strcmp(string, STR_CAM_OUT_FMT_H263)) {

      *format = CAM_FMT_H263;
    }
    else if(!strcmp(string, STR_CAM_OUT_FMT_H265)) {

      *format = CAM_FMT_H265;
    }
    else if(!strcmp(string, STR_CAM_OUT_FMT_VP8)) {

      *format = CAM_FMT_VP8;
    }
    else if(!strcmp(string, STR_CAM_OUT_FMT_VP9)) {

      *format = CAM_FMT_VP9;
    }
    else {

      *format = CAM_FMT_UNK;

      retval = -1;
    }
  }
  else {

    createLogMessage(STR_LOG_MSG_FUNC2_ARG_INVAL, LOG_SVRTY_WRN);
    retval = -1;
  }

  return retval;
}

/**
 * @brief       Prints the capabilities of video encoding formats.
 * 
 * @details     Prints the capabilities of supported video encoding
 *              formats on the standard output.
 * 
 * @note        This function is recommended for diagnostics.
 * 
 * @param[in]   capabilities Array containing capabilities of video encoding formats.
 * @param[out]  size Size of capabilities array.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
#ifdef CC_DEBUG_MODE
static int printVideoCodingFormatCaps(const VideoCodingFormatCaps_T capabilities[], const size_t size) {

  int capIndex = 0; 
  int retval = 0;

  if(NULL != capabilities) {

    fprintf(stdout, STR_FORMAT_FMTS_CAPS_TITLE);

    for(capIndex = 0; capIndex < size; ++capIndex) {

      if(capabilities[capIndex].supported) {

        switch(capIndex) {

          case CAM_FMT_H265:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_H265);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_H264:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_H264);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_VP8:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_VP8);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_VP9:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_VP9);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_JPEG:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_JPEG);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_H263:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_H263);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_RAW:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_RAW);
            fprintf(stdout, STR_FORMAT_CAPS_COMMON,
                      capabilities[capIndex].width, capabilities[capIndex].height,
                        capabilities[capIndex].framerateNumerator,
                          capabilities[capIndex].framerateDenominator);
            fprintf(stdout, "\n");
            break;

          case CAM_FMT_MPEG:

            // MPEG format currently not supported by software
            break;

          case CAM_FMT_MPEGTS:

            // MPEGTS format currently not supported by software
            break;

          case CAM_FMT_BAYER:

            // BAYER format currently not supported by software
            break;

          case CAM_FMT_DV:

            // DV format currently not supported by software
            break;

          case CAM_FMT_FWHT:

            // FWHT format currently not supported by software
            break;

          case CAM_FMT_PWC1:

            // PWC1 format currently not supported by software
            break;

          case CAM_FMT_PWC2:

            // PWC2 format currently not supported by software
            break;

          case CAM_FMT_SONIX:

            // SONIX format currently not supported by software
            break;

          case CAM_FMT_WMV:

            // WMV format currently not supported by software
            break;

          default:

            fprintf(stdout, STR_FORMAT_FMT_GENERAL, STR_CAM_OUT_FMT_UNK);
            fprintf(stdout, "\n");
            break;
        }
      }
    }

    fflush(stdout);
  }
  else {

    createLogMessage(STR_LOG_MSG_FUNC3_ARG_INVAL, LOG_SVRTY_WRN);
    retval = -1;
  }

  return retval;
}
#endif

int getCameraCapabilities(GstElement *v4l2srcElement, VideoCodingFormatContext_T *ctx) {

  int retval = 0;
  int width, height, framerateNum, framerateDenom;
  int listElemIndex;
  guint i = 0;
  GstPad *sourcePad = NULL;
  GstCaps *capabilities = NULL;
  GstStructure *capsStructure = NULL;
  const GValue *framerateFract = NULL;
  const GValue *framerateList = NULL;
  VideoCodingFormat_T selectedFormat = CAM_FMT_UNK;

  if((NULL != v4l2srcElement) && (NULL != ctx)) {

    memset(ctx->capsArray, 0, sizeof(VideoCodingFormatCaps_T)*ctx->size);

    sourcePad = gst_element_get_static_pad(v4l2srcElement, "src");
    if(NULL != sourcePad ) {

      capabilities = gst_pad_query_caps(sourcePad, NULL);
      if(NULL != capabilities) {

        if(gst_caps_is_any(capabilities)) {
        
          createLogMessage(STR_LOG_MSG_FUNC5_CAPS_ANY, LOG_SVRTY_ERR);
          retval = -1;
        }
        else if(gst_caps_is_empty(capabilities)) {
          
          createLogMessage(STR_LOG_MSG_FUNC5_CAPS_EMPTY, LOG_SVRTY_ERR);
          retval = -1;
        }
        else {

          /* Iterate over camera output formats */
          for (i = 0; i < gst_caps_get_size(capabilities); ++i) {

            capsStructure = gst_caps_get_structure(capabilities, i);

            stringToVideoCodingFormat(gst_structure_get_name(capsStructure), &selectedFormat);
            if(ctx->size > selectedFormat) {
              
              /* Update capabilities of supported format */
              width = height = 0;
              framerateNum = framerateDenom = 0;
              framerateFract = NULL;
              framerateList = NULL;

              ctx->capsArray[selectedFormat].supported = CAM_FMT_SUPPORTED;
              gst_structure_get_int(capsStructure, "width", &width);
              gst_structure_get_int(capsStructure, "height", &height);

              /* Check update condition (best resolution) */
              if(
                (width * height)
                >
                (ctx->capsArray[selectedFormat].width * ctx->capsArray[selectedFormat].height)
              ) {

                ctx->capsArray[selectedFormat].width = width;
                ctx->capsArray[selectedFormat].height = height;

                framerateList = gst_structure_get_value(capsStructure, "framerate");
                for(listElemIndex = 0; listElemIndex < gst_value_list_get_size(framerateList); ++listElemIndex) {

                  framerateFract = gst_value_list_get_value(framerateList, listElemIndex);
                  if(
                    (
                    (float)(((float)gst_value_get_fraction_numerator(framerateFract))/((float)gst_value_get_fraction_denominator(framerateFract)))
                    >
                    (float)(((float)framerateNum)/((float)framerateDenom))
                    )

                    ||

                    (0 == framerateDenom)
                  ) {

                    framerateNum = gst_value_get_fraction_numerator(framerateFract);
                    framerateDenom = gst_value_get_fraction_denominator(framerateFract);
                  }
                }

                ctx->capsArray[selectedFormat].framerateNumerator = framerateNum;
                ctx->capsArray[selectedFormat].framerateDenominator = framerateDenom;
              }
            }
          }
        }

        gst_caps_unref(capabilities);
        gst_object_unref(sourcePad);
      }
      else {

        createLogMessage(STR_LOG_MSG_FUNC5_CAPS_RTRV_FAIL, LOG_SVRTY_ERR);
        gst_object_unref(sourcePad);
        retval = -1;
      }
    }
    else {

      createLogMessage(STR_LOG_MSG_FUNC5_SRCPAD_RTRV_FAIL, LOG_SVRTY_ERR);
      retval = -1;
    }
  }
  else {

    createLogMessage(STR_LOG_MSG_FUNC5_ARG_INVAL, LOG_SVRTY_ERR);
    retval = -1;
  }

  return retval;
}

/**
 * @brief       Prints V4L2 device capabilities on the standard output.
 * 
 * @details     Prints the capabilities of the given V4L2
 *              compatible device on the standard output.
 * 
 * @note        Displayed capabilities indicate only the
 *              device node capabilities and not the
 *              physical device's capabilities.
 * 
 * @param[in]   capabilities V4L2 device capabilities.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
static int printDeviceCapabilities(const struct v4l2_capability *capabilities) {
    
    int retval = 0;
    
    if(NULL != capabilities) {
        
        fflush(stdout);
        fprintf(
            
            stdout,
            STR_FORMAT_DEV_CAPS,
            capabilities->driver,
            capabilities->version,
            capabilities->card,
            capabilities->bus_info,
            capabilities->device_caps
        );
        fflush(stdout);
    }
    else {
        
        retval = -1;
    }
    
    return retval;
}

int getCameraDevicePath(char cameraDevicePath[], const size_t size) {
    
    int deviceFileDescriptor;
    int retval = 0;
    char devicePath[STR_SIZE_DEV_PATH] = {0};
    DIR *deviceDirectory = NULL;
    struct dirent *deviceEntry = NULL;
    struct v4l2_capability videoDeviceCapabilities = {0};
    
    /* Check input arguments */
    if((NULL == cameraDevicePath) || (0 == size)) {
     
        createLogMessage(STR_LOG_MSG_FUNC1_ARG_INVAL, LOG_SVRTY_ERR);
        
        retval = -1;
        return retval;
    }
    
    /* Search camera device under /dev directory */
    deviceDirectory = opendir(STR_DEV_DIR_PATH);
    if(NULL == deviceDirectory) {
        
        #ifdef CC_DEBUG_MODE
        perror("opendir");
        fflush(stderr);
        #endif
        createLogMessage(STR_LOG_MSG_FUNC1_OPEN_DIR_FAIL, LOG_SVRTY_ERR);

        retval = -1;
    }
    else {
        
        while(NULL != (deviceEntry = readdir(deviceDirectory))) {
        
            /* Filter devices by name */
            if(!strncmp(deviceEntry->d_name, STR_HINT_CAM_DEV_NAME, strlen(STR_HINT_CAM_DEV_NAME))) {
            
                /* Construct device path string */
                strcpy(devicePath, STR_DEV_DIR_PATH);
                strcat(devicePath, "/");
                strcat(devicePath, deviceEntry->d_name);
                
                deviceFileDescriptor = open(devicePath, O_RDWR | O_NONBLOCK, 0644);
                if(deviceFileDescriptor < 0) {
                
                    #ifdef CC_DEBUG_MODE
                    perror("open");
                    fflush(stderr);
                    fprintf(stdout, STR_LOG_MSG_FUNC1_OPEN_DEV_FAIL, deviceEntry->d_name);
                    fflush(stdout);
                    #endif
                    syslog(LOG_DAEMON | LOG_WARNING, STR_LOG_MSG_FUNC1_OPEN_DEV_FAIL, deviceEntry->d_name);
                }
                else {
             
                    /* Check V4L2 compatibility */
                    if(0 > ioctl(deviceFileDescriptor, VIDIOC_QUERYCAP, &videoDeviceCapabilities)) {
                        
                        #ifdef CC_DEBUG_MODE
                        perror("ioctl");
                        fflush(stderr);
                        fprintf(stdout, STR_LOG_MSG_FUNC1_QUERY_CAP_FAIL, deviceEntry->d_name);
                        fflush(stdout);
                        #endif
                        syslog(LOG_DAEMON | LOG_WARNING, STR_LOG_MSG_FUNC1_QUERY_CAP_FAIL, deviceEntry->d_name);
                    }
                    else {
                        
                        /* Check Video Capture capability */
                        if(videoDeviceCapabilities.device_caps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
                            
                            memset(cameraDevicePath, 0, size);
                            strncpy(cameraDevicePath, devicePath, size-1);
                            close(deviceFileDescriptor);
                            closedir(deviceDirectory);
                            
                            retval = 0;
                            return retval;
                        }
                    }
                    
                    close(deviceFileDescriptor);
                }
            }
        }
    }
    
    closedir(deviceDirectory);
    
    retval = -1;
    return retval;
}