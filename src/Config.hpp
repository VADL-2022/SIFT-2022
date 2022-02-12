//
//  Config.hpp
//  SIFT
//
//  Created by VADL on 11/10/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef Config_h
#define Config_h

#include "SIFT.hpp"

// Config //

// Config: Define these in the Makefile instead of here: //
// SIFT implementation
//#define SIFT_IMPL SIFTAnatomy
//#define SIFTAnatomy_
//#define SIFT_IMPL SIFTOpenCV
//#define SIFTOpenCV_
//#define SIFT_IMPL SIFTGPU
//#define SIFTGPU_
// //


//#define SLEEP_BEFORE_RUNNING 30 * 3000 // Milliseconds
//#define STOP_AFTER 30 * 3000 // Milliseconds
// //

// Config: Define these in the Makefile, i.e. with `CFLAGS += -DUSE_COMMAND_LINE_ARGS` instead, although these can be overriden here: //
//#define USE_COMMAND_LINE_ARGS
// //

#ifdef USE_COMMAND_LINE_ARGS
struct CommandLineConfig {
    bool cameraTestOnly = false, imageCaptureOnly = false, imageFileOutput = false, siftVideoOutput = false, folderDataSource = false, videoFileDataSource = false, mainMission = false, noPreviewWindow = false, useSetTerminate = true, verbose = false;
    int flushVideoOutputEveryNSeconds = 2; // Default is to flush after every 2 seconds
    
    bool showPreviewWindow() const {
        return !noPreviewWindow;
    }
    
    bool cameraDataSource() const {
        return !folderDataSource && !videoFileDataSource;
    }
    
    bool shouldFlushVideoOutputEveryNSeconds() const {
        return flushVideoOutputEveryNSeconds != -1;
    }
};
#endif

#ifdef USE_COMMAND_LINE_ARGS
#define COMPARE_KEYPOINTS_ADDITIONAL_ARGS , cfg
#define COMPARE_KEYPOINTS_ADDITIONAL_PARAMS , const CommandLineConfig& cfg
#define CMD_CONFIG(x) cfg.x
#else
#define CMD_CONFIG(x) true
#define COMPARE_KEYPOINTS_ADDITIONAL_ARGS
#define COMPARE_KEYPOINTS_ADDITIONAL_PARAMS
#endif

struct SIFTAnatomy;
struct SIFTOpenCV;
struct SIFTGPU;
using SIFT_T = SIFT_IMPL;

#ifdef USE_COMMAND_LINE_ARGS
#define MaybeVirtual virtual
#define MaybePureVirtual = 0
#define MaybeOverride override
#else
#define MaybeVirtual
#define MaybePureVirtual
#define MaybeOverride
#endif

#endif /* Config_h */
