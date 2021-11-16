//
//  siftMain.hpp
//  SIFT
//
//  Created by VADL on 11/15/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef siftMain_h
#define siftMain_h

#include <opencv2/highgui.hpp>
#include "Config.hpp"

#include "lib/ctpl_stl.hpp"
extern ctpl::thread_pool tp;

#ifdef USE_COMMAND_LINE_ARGS
extern cv::Rect g_desktopSize;
#endif

void stopMain();
bool stoppedMain();

#endif /* siftMain_h */
