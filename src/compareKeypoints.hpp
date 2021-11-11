//
//  compareKeypoints.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef compareKeypoints_hpp
#define compareKeypoints_hpp

#include "common.hpp"
#include "DataOutput.hpp"
#include "Config.hpp"

// Forward declarations
struct SIFTState;
struct SIFTParams;

#ifdef USE_COMMAND_LINE_ARGS
#define COMPARE_KEYPOINTS_ADDITIONAL_ARGS , cfg
#define COMPARE_KEYPOINTS_ADDITIONAL_PARAMS , const CommandLineConfig& cfg
#define CMD_CONFIG(x) cfg.x
#else
#define CMD_CONFIG(x) true
#endif
// Returns true to mean you need to retry with the modified params (modified by this function) on the previous file.
bool compareKeypoints(cv::Mat& canvas, SIFTState& s, SIFTParams& p, struct sift_keypoints* keypoints, cv::Mat& backtorgb COMPARE_KEYPOINTS_ADDITIONAL_PARAMS);

#endif /* compareKeypoints_hpp */
