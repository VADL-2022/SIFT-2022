//
//  DataOutput.hpp
//  SIFT
//
//  Created by VADL on 11/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef DataOutput_hpp
#define DataOutput_hpp

#include <opencv2/opencv.hpp>
#include "common.hpp"
#include "DataSource.hpp"
#include "KeypointsAndMatching.hpp"

template <typename Wrapped>
struct DataOutputBase {
    void output(cv::Mat& m);
};

struct PreviewWindowDataOutput : public DataOutputBase<PreviewWindowDataOutput>
{
    void init(size_t width, size_t height);
    void showCanvas(std::string name);
    void showCanvas(std::string name, cv::Mat& canvas);
    
    void run(FolderDataSource& src, SIFTState& s, SIFTParams& p, cv::Mat& backtorgb, struct sift_keypoints* keypoints, bool retryNeeded, size_t& index, int n);
    
    cv::Mat canvas;
};

struct FileDataOutput : public DataOutputBase<FileDataOutput>
{
    
};

#define ARGS FolderDataSource& src, SIFTState& s, SIFTParams& p, cv::Mat& backtorgb, struct sift_keypoints* keypoints, bool retryNeeded, size_t& index, int n
template<class T>
extern void runDataOutput(T& o, ARGS) { throw 0; }
template<>
void runDataOutput(PreviewWindowDataOutput& o, ARGS);
//template<>
//void runDataOutput(FileDataOutput& o, ARGS);

#endif /* DataOutput_hpp */
