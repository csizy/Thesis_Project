/**
 * @file        stream_utils.h
 * @author      Adam Csizy
 * @date        2021-04-05
 * @version     v1.1.0
 * 
 * @brief       Streaming utilities and video streaming module
 */

#pragma once


#include "com_utils.h"


/* Streaming related global variable declarations */

extern ModuleMessageQueue_T streamMsgq;     /**< Module message queue of the video streaming module */


/* Streaming related public function declarations */

/**
 * @brief       Initialize streaming module.
 * 
 * @details     Initializes the streaming module's message
 *              queue and starts the streaming control thread.
 * 
 * @return      Result of execution.
 * 
 * @retval      0 Success
 * @retval      -1 Failure
 */
int initStreamModule(void);