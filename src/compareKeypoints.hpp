#ifndef compareKeypoints_hpp
#define compareKeypoints_hpp

#include "common.hpp"

// Forward declarations
struct SIFTState;
struct SIFTParams;

// Returns true to mean you need to retry with the modified params (modified by this function) on the previous file.
bool compareKeypoints(SIFTState& s, SIFTParams& p, struct sift_keypoints* keypoints, cv::Mat& backtorgb);

#endif /* compareKeypoints_hpp */
