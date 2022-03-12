//
//  siftMainCanvasShower.cpp
//  SIFT
//
//  Created by VADL on 3/10/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "siftMainCanvasShower.hpp"

#include "../siftMain.hpp"
#include "../common.hpp"
#include "../DataOutput.hpp"

#ifdef USE_COMMAND_LINE_ARGS
Queue<ProcessedImage<SIFT_T>, 16> canvasesReadyQueue;

cv::Mat prepareCanvas(ProcessedImage<SIFT_T>& img) {
    cv::Mat c = img.canvas.empty() ? img.image : img.canvas;
    if (CMD_CONFIG(showPreviewWindow())) {
        // Draw the image index
        // https://stackoverflow.com/questions/46500066/how-to-put-a-text-in-an-image-in-opencv-c/46500123
        cv::putText(c, //target image
                    std::to_string(img.i), //text
                    cv::Point(10, 25), //top-left position
                    //cv::Point(10, c.rows / 2), //center position
                    cv::FONT_HERSHEY_DUPLEX,
                    1.0,
                    CV_RGB(118, 185, 0), //font color
                    2);
        
        // IMU data, if any
        if (img.imu) {
            cv::putText(c,
                        std::to_string(img.imu->yprNed.x) + "," + std::to_string(img.imu->yprNed.y) + "," + std::to_string(img.imu->yprNed.z),
                        cv::Point(10, c.rows - 20),
                        cv::FONT_HERSHEY_DUPLEX,
                        1.0,
                        CV_RGB(118, 185, 0), //font color
                        2);
        }
    }
    return c;
}

void showAnImageUsingCanvasesReadyQueue(DataSourceBase* src, DataOutputBase& o2) {
        // Show images if we have them and if we are showing a preview window
        if (!canvasesReadyQueue.empty() && CMD_CONFIG(showPreviewWindow())) {
            ProcessedImage<SIFT_T> img;
            canvasesReadyQueue.dequeue(&img);
            cv::Mat realCanvas = prepareCanvas(img);
            commonUtils::imshow("", realCanvas); // Canvas can be empty if no matches were done on the image, hence nothing was rendered. // TODO: There may be some keypoints but we don't show them..
            if (CMD_CONFIG(siftVideoOutput)) {
                // Save frame with SIFT keypoints rendered on it to the video output file
                cv::Rect rect = src->shouldCrop() ? src->crop() : cv::Rect();
                if (img.canvas.empty()) {
                    std::cout << "Canvas has empty image, using original\n";
                    o2.showCanvas("", img.image, false, rect.empty() ? nullptr : &rect);
                }
                else {
                    o2.showCanvas("", img.canvas, false, rect.empty() ? nullptr : &rect);
                }
            }
            //cv::waitKey(30);
            auto size = canvasesReadyQueue.size();
            { out_guard();
                std::cout << "Showing image from canvasesReadyQueue with " << size << " images left" << std::endl; }
            char c = cv::waitKey(CMD_CONFIG(waitKeyForever) ? 0 : (1 + 150.0 / (1 + size))); // Sleep less as more come in
            if (c == 'q') {
                // Quit
                { out_guard();
                    std::cout << "Exiting (q pressed)" << std::endl; }
                stopMain();
            }
        }
}
#endif
