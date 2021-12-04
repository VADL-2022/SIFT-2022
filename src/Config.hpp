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
// SIFT implementation
#define SIFT_IMPL SIFTAnatomy
#define SIFTAnatomy_
//#define SIFT_IMPL SIFTOpenCV
//#define SIFTOpenCV_


//#define SLEEP_BEFORE_RUNNING 30 * 3000 // Milliseconds
//#define STOP_AFTER 30 * 3000 // Milliseconds
// //

// Config: Define these in the Makefile, i.e. with `CFLAGS += -DUSE_COMMAND_LINE_ARGS` instead, although these can be overriden here: //
//#define USE_COMMAND_LINE_ARGS
// //

#ifdef USE_COMMAND_LINE_ARGS
struct CommandLineConfig {
    bool imageCaptureOnly = false, imageFileOutput = false, folderDataSource = false, videoFileDataSource = false, mainMission = false, noPreviewWindow;
    
    bool showPreviewWindow() const {
        return !noPreviewWindow;
    }
    
    bool cameraDataSource() const {
        return !folderDataSource && !videoFileDataSource;
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
using SIFT_T = SIFT_IMPL;

#ifdef USE_COMMAND_LINE_ARGS
#define MaybeVirtual virtual
#define MaybePureVirtual = 0
#else
#define MaybeVirtual
#define MaybePureVirtual
#endif

#endif /* Config_h */
