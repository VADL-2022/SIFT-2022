//
//  common.cpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "common.hpp"

Timer t;

#pragma mark Drawing

void drawCircle(cv::Mat& img, cv::Point cp, int radius)
{
    //cv::Scalar black( 0, 0, 0 );
    cv::Scalar color = nextRNGColor();
    
    cv::circle( img, cp, radius, color );
}
void drawRect(cv::Mat& img, cv::Point center, cv::Size2f size, float orientation_degrees, int thickness) {
    cv::RotatedRect rRect = cv::RotatedRect(center, size, orientation_degrees);
    cv::Point2f vertices[4];
    rRect.points(vertices);
    cv::Scalar color = nextRNGColor();
    //printf("%f %f %f\n", color[0], color[1], color[2]);
    for (int i = 0; i < 4; i++)
        cv::line(img, vertices[i], vertices[(i+1)%4], color, thickness);
    
    // To draw a rectangle bounding this one:
    //Rect brect = rRect.boundingRect();
    //cv::rectangle(img, brect, nextRNGColor(), 2);
}
void drawSquare(cv::Mat& img, cv::Point center, int size, float orientation_degrees, int thickness) {
    drawRect(img, center, cv::Size2f(size, size), orientation_degrees, thickness);
}

#pragma mark imshow

// Wrapper around imshow that reuses the same window each time
void imshow(std::string name, cv::Mat& mat) {
    static std::optional<std::string> prevWindowTitle;
    
    if (prevWindowTitle) {
        // Rename window
        cv::setWindowTitle(*prevWindowTitle, name);
    }
    else {
        prevWindowTitle = name;
        cv::namedWindow(name, cv::WINDOW_AUTOSIZE); // Create Window
        
//        // Set up window
//        char TrackbarName[50];
//        sprintf( TrackbarName, "Alpha x %d", alpha_slider_max );
//        cv::createTrackbar( TrackbarName, name, &alpha_slider, alpha_slider_max, on_trackbar );
//        on_trackbar( alpha_slider, 0 );
    }
    cv::imshow(*prevWindowTitle, mat); // prevWindowTitle (the first title) is always used as an identifier for this window, regardless of the renaming done via setWindowTitle().
}

#pragma mark RNG

// Based on https://stackoverflow.com/questions/31658132/c-opencv-not-drawing-circles-on-mat-image and https://stackoverflow.com/questions/19400376/how-to-draw-circles-with-random-colors-in-opencv/19401384
#define RNG_SEED 12345
cv::RNG rng(RNG_SEED); // Random number generator
cv::Scalar lastColor;
void resetRNG() {
    rng = cv::RNG(RNG_SEED);
}
cv::Scalar nextRNGColor() {
    //return cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255)); // Most of these are rendered as close to white for some reason..
    //return cv::Scalar(0,0,255); // BGR color value (not RGB)
    lastColor = cv::Scalar(rng.uniform(0, 255) / rng.uniform(1, 255),
                   rng.uniform(0, 255) / rng.uniform(1, 255),
                   rng.uniform(0, 255) / rng.uniform(1, 255));
    return lastColor;
}
