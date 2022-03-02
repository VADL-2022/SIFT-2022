//
//  siftMainSIFT.cpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "siftMainSIFT.hpp"

thread_local SIFT_T sift;
Queue<ProcessedImage<SIFT_T>, processedImageQueueBufferSize> processedImageQueue;
cv::Mat lastImageToFirstImageTransformation; // "Message box" for the accumulated transformations so far
std::mutex lastImageToFirstImageTransformationMutex;
