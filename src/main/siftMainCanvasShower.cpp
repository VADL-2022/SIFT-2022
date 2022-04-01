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
#include "../utils.hpp"

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
        
        // Show transformation
        // Transform the four corner points of the rect:
        
        cv::Mat& img_object = img.image, &img_matches = c;
        cv::Mat H = img.transformation; //cv::Mat::eye(3, 3, CV_64F); //img.transformation;
        if (H.empty()) return c;
        H = H.inv();
        
//        cv::Ptr<cv::Formatted> str;
//        matrixToString(H, str);
//        { out_guard();
//            std::cout << "H: " << str << std::endl; }
        
        // https://docs.opencv.org/2.4/doc/tutorials/features2d/feature_homography/feature_homography.html
        //-- Get the corners from the image_1 ( the object to be "detected" )
        std::vector<cv::Point2f> obj_corners(4);
        obj_corners[0] = cv::Point2f(0,0); obj_corners[1] = cv::Point2f( img_object.cols, 0 );
        obj_corners[2] = cv::Point2f( img_object.cols, img_object.rows ); obj_corners[3] = cv::Point2f( 0, img_object.rows );
        std::vector<cv::Point2f> scene_corners(4);
        
        //cv::perspectiveTransform(obj_corners, scene_corners, H.inv());

        //-- Draw lines between the corners (the mapped object in the scene - image_2 )
        cv::line( img_matches, scene_corners[0] + cv::Point2f( img_object.cols, 0), scene_corners[1] + cv::Point2f( img_object.cols, 0), cv::Scalar(0, 255, 0), 4 );
        cv::line( img_matches, scene_corners[1] + cv::Point2f( img_object.cols, 0), scene_corners[2] + cv::Point2f( img_object.cols, 0), cv::Scalar( 0, 255, 0), 4 );
        cv::line( img_matches, scene_corners[2] + cv::Point2f( img_object.cols, 0), scene_corners[3] + cv::Point2f( img_object.cols, 0), cv::Scalar( 0, 255, 0), 4 );
        cv::line( img_matches, scene_corners[3] + cv::Point2f( img_object.cols, 0), scene_corners[0] + cv::Point2f( img_object.cols, 0), cv::Scalar( 0, 255, 0), 4 );
        
        // Draw inner lines to help tell the true orientation of the box
        float INSET = 10.0f;
        scene_corners[0].x += INSET;
        scene_corners[0].y -= INSET;
        scene_corners[1].x += INSET;
        scene_corners[1].y -= INSET;
        scene_corners[2].x += INSET;
        scene_corners[2].y -= INSET;
        scene_corners[3].x += INSET;
        scene_corners[3].y -= INSET;
        cv::line( img_matches, scene_corners[0] + cv::Point2f( img_object.cols, 0), scene_corners[1] + cv::Point2f( img_object.cols, 0), cv::Scalar(255, 100, 0), 2 );
        cv::line( img_matches, scene_corners[1] + cv::Point2f( img_object.cols, 0), scene_corners[2] + cv::Point2f( img_object.cols, 0), cv::Scalar( 255, 100, 0), 2 );
        cv::line( img_matches, scene_corners[2] + cv::Point2f( img_object.cols, 0), scene_corners[3] + cv::Point2f( img_object.cols, 0), cv::Scalar( 255, 100, 0), 2 );
        cv::line( img_matches, scene_corners[3] + cv::Point2f( img_object.cols, 0), scene_corners[0] + cv::Point2f( img_object.cols, 0), cv::Scalar( 255, 100, 0), 2 );
        
        //drawRect(c, {0,0}, {c.cols, c.rows}, <#float orientation_degrees#>)
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
            general.attr("drainPreviewWindowQueue")(); // Show some Python stuff too
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
