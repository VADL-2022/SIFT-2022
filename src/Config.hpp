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
//#define SIFT_IMPL SIFTAnatomy
#define SIFT_IMPL SIFTOpenCV
#define SIFTOpenCV_
// //

// Config: Define these in the Makefile, i.e. with `CFLAGS += -DUSE_COMMAND_LINE_ARGS` instead, although these can be overriden here: //
//#define USE_COMMAND_LINE_ARGS
// //

#ifdef USE_COMMAND_LINE_ARGS
struct CommandLineConfig {
    bool imageCaptureOnly = false, imageFileOutput = false, folderDataSource = false, mainMission = false;
};
#endif

struct SIFTAnatomy;
struct SIFTOpenCV;
using SIFT_T = SIFT_IMPL;

#endif /* Config_h */
