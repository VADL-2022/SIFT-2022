//
//  siftMainCmdConfig.hpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef siftMainCmdConfig_hpp
#define siftMainCmdConfig_hpp

#include "../Config.hpp"
#include "../SIFT.hpp"
#include "../Queue.hpp"

#ifdef USE_COMMAND_LINE_ARGS
struct DataSourceBase;
struct DataOutputBase;
cv::Mat prepareCanvas(ProcessedImage<SIFT_T>& img);
void showAnImageUsingCanvasesReadyQueue(DataSourceBase* src, DataOutputBase& o2);

extern Queue<ProcessedImage<SIFT_T>, 16> canvasesReadyQueue;
extern CommandLineConfig cfg;

//#include <type_traits>
#endif

#endif /* siftMainCmdConfig_hpp */
