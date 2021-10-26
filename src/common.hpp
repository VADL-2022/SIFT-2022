#ifndef common_hpp
#define common_hpp

#include "Includes.hpp"

extern Timer t;

void drawCircle(cv::Mat& img, cv::Point cp, int radius);
void drawRect(cv::Mat& img, cv::Point center, cv::Size2f size, float orientation_degrees, int thickness = 2);
void drawSquare(cv::Mat& img, cv::Point center, int size, float orientation_degrees, int thickness = 2);

// Wrapper around imshow that reuses the same window each time
void imshow(std::string name, cv::Mat& mat);

extern cv::RNG rng; // Random number generator
extern cv::Scalar lastColor;
void resetRNG();
cv::Scalar nextRNGColor();

#endif /* common_hpp */
