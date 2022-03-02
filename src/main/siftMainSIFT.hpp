//
//  siftMainSIFT.hpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef siftMainSIFT_hpp
#define siftMainSIFT_hpp

#include "../Config.hpp"
#include "../KeypointsAndMatching.hpp"
#include "../Queue.hpp"

extern thread_local SIFT_T sift;
constexpr size_t processedImageQueueBufferSize = 1024 /*256*/ /*32*/;
extern Queue<ProcessedImage<SIFT_T>, processedImageQueueBufferSize> processedImageQueue;
extern cv::Mat lastImageToFirstImageTransformation; // "Message box" for the accumulated transformations so far
extern std::mutex lastImageToFirstImageTransformationMutex;

#endif /* siftMainSIFT_hpp */
