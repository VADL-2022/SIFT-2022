//
//  DataOutput.cpp
//  SIFT
//
//  Created by VADL on 11/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "DataOutput.hpp"

#include "opencv2/highgui.hpp"

void PreviewWindowDataOutput::init(size_t width, size_t height) {
    // Initialize canvas if needed
    if (canvas.data == nullptr) {
        puts("Init canvas");
        canvas = cv::Mat(height, width, CV_32FC4);
    }
}

void PreviewWindowDataOutput::showCanvas(std::string name) {
    showCanvas(name, canvas);
}
void PreviewWindowDataOutput::showCanvas(std::string name, cv::Mat& canvas) {
    t.reset();
    imshow(name, canvas);
    t.logElapsed("show canvas window");
}

#undef ARGS
