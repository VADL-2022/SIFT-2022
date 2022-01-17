//
//  SIFT.hpp
//  SIFT
//
//  Created by VADL on 10/22/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef SIFT_hpp
#define SIFT_hpp

#include "common.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "KeypointsAndMatching.hpp"

struct CommandLineConfig;

template <typename SIFT_Type>
struct ProcessedImage {};

// Forward declarations //
struct SIFTAnatomy;
struct SIFTOpenCV;
struct SIFTGPU;

struct DataSourceBase;
// //

enum MatchResult: uint8_t {
    Success = 0,
    NotEnoughMatchesForFirstImage = 1,
    NotEnoughMatchesForSecondImage = 2,
    NotEnoughKeypoints = 3 // Should be handled outside findKeypoints but we report it here as a courtesy
};

#ifdef SIFTAnatomy_
template <>
struct ProcessedImage<SIFTAnatomy> {
    ProcessedImage() = default;
    
    // Insane move assignment operator
    // move ctor: ProcessedImage(ProcessedImage&& other) {
//    ProcessedImage& operator=(ProcessedImage&& other) {
//        //bad whole:
//        image = std::move(other.image);
//        computedKeypoints.reset(other.computedKeypoints.release());
//        out_k1.reset(other.out_k1.release());
//        out_k2A.reset(other.out_k2A.release());
//        out_k2B.reset(other.out_k2B.release());
//        transformation = std::move(other.transformation);
//        new (&other.transformation) cv::Mat();
//        p = std::move(other.p);
//        other.p.params = nullptr;
//        other.p.paramsName = nullptr;
//
//        return *this;
//    }
    
//    ProcessedImage& operator=(ProcessedImage& other) {
//        image = other.image;
//        // bad:
//        computedKeypoints.reset(other.computedKeypoints.get());
//        // bad:
//        out_k1.reset(other.out_k1.get());
//        // bad:
//        out_k2A.reset(other.out_k2A.get());
//        // bad:
//        out_k2B.reset(other.out_k2B.get());
//        transformation = other.transformation;
//        // bad:
//        p = other.p;
//
//        return *this;
//    }
    
    cv::Mat image;
    
    shared_keypoints_ptr_t computedKeypoints;
    std::shared_ptr<struct sift_keypoint_std> k;
    int n; // Number of keypoints in `k`
    
    // Matching with the previous image, if any //
    shared_keypoints_ptr_t out_k1;
    shared_keypoints_ptr_t out_k2A;
    shared_keypoints_ptr_t out_k2B;
    cv::Mat transformation;
    void applyDefaultMatchingParams() {
        p.meth_flag = 1;
        p.thresh = 0.6;
    }
    void resetMatching() {
        // Free memory and mark this as an unused ProcessedImage:
        out_k1.reset();
        out_k2A.reset();
        out_k2B.reset();
    }
    // //
    
    SIFTParams p;
    size_t i;

#ifdef USE_COMMAND_LINE_ARGS
    cv::Mat canvas;
#endif
};

#elif defined(SIFTOpenCV_)
template <>
struct ProcessedImage<SIFTOpenCV> {
    ProcessedImage() = default;
    
    cv::Mat image;
    
    std::vector<cv::KeyPoint> computedKeypoints;
    cv::Mat descriptors;
    
    // Matching with the previous image, if any //
    std::vector< cv::DMatch > matches;
    cv::Mat transformation;
    void applyDefaultMatchingParams() {}
    void resetMatching() {
        computedKeypoints.clear();
        matches.clear();
    }
    // //
    
#ifdef USE_COMMAND_LINE_ARGS
    cv::Mat canvas;
#endif
};

#elif defined(SIFTGPU_)
#include "SiftGPU.h"
template <>
struct ProcessedImage<SIFTGPU> {
    ProcessedImage() = default;
    
    cv::Mat image;
    
    std::vector<SiftGPU::SiftKeypoint> keys1;
    std::vector<float> descriptors1;
    int num1;
    
//    shared_keypoints_ptr_t computedKeypoints;
//    std::shared_ptr<struct sift_keypoint_std> k;
//    int n; // Number of keypoints in `k`
//
//    // Matching with the previous image, if any //
//    shared_keypoints_ptr_t out_k1;
//    shared_keypoints_ptr_t out_k2A;
//    shared_keypoints_ptr_t out_k2B;
    cv::Mat transformation;
    void applyDefaultMatchingParams() {
//        p.meth_flag = 1;
//        p.thresh = 0.6;
    }
    void resetMatching() {
        // Free memory and mark this as an unused ProcessedImage:
//        out_k1.reset();
//        out_k2A.reset();
//        out_k2B.reset();
    }
    // //
    
    SIFTParams p;
    size_t i;

#ifdef USE_COMMAND_LINE_ARGS
    cv::Mat canvas;
#endif
};
#endif


struct SIFTBase {
    
};

#ifdef SIFTAnatomy_
struct SIFTAnatomy : public SIFTBase {
    SIFTAnatomy() {
        
    }
    
    std::pair<sift_keypoints* /*keypoints*/, std::pair<sift_keypoint_std* /*k*/, int /*n*/>> findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale);
    
    // Finds matching and homography
    // Returns one of the MatchResult::NotEnoughDescriptors[...] if not enough descriptors.
    MatchResult findHomography(ProcessedImage<SIFTAnatomy>& img1, ProcessedImage<SIFTAnatomy>& img2, SIFTState& s
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
#endif
                        );
};

#elif defined(SIFTOpenCV_)
struct SIFTOpenCV : public SIFTBase {
    // Defaults //
//    int     nfeatures = 0;
//    int     nOctaveLayers = 3;
//    double     contrastThreshold = 0.04;
//    double     edgeThreshold = 10;
//    double     sigma = 1.6;
    // //
    // Testing //
    int     nfeatures = 200;
    int     nOctaveLayers = 5;
    double     contrastThreshold = 0.03;
    double     edgeThreshold = 12;
    double     sigma = 1.2;
    // //
    
    // https://github.com/opencv/opencv/blob/17234f82d025e3bbfbf611089637e5aa2038e7b8/modules/features2d/test/test_sift.cpp
    // :
    /* https://docs.opencv.org/3.4/d7/d60/classcv_1_1SIFT.html :
     static Ptr<SIFT> cv::SIFT::create    (    int     nfeatures,
     int     nOctaveLayers,
     double     contrastThreshold,
     double     edgeThreshold,
     double     sigma,
     int     descriptorType
     )
     */
    /* https://docs.opencv.org/3.4/d7/d60/classcv_1_1SIFT.html : more useful part:
     static Ptr<SIFT> cv::SIFT::create    (    int     nfeatures = 0,
     int     nOctaveLayers = 3,
     double     contrastThreshold = 0.04,
     double     edgeThreshold = 10,
     double     sigma = 1.6
     
     Docs:
     Parameters
     nfeatures    The number of best features to retain. The features are ranked by their scores (measured in SIFT algorithm as the local contrast)
     nOctaveLayers    The number of layers in each octave. 3 is the value used in D. Lowe paper. The number of octaves is computed automatically from the image resolution.
     contrastThreshold    The contrast threshold used to filter out weak features in semi-uniform (low-contrast) regions. The larger the threshold, the less features are produced by the detector.
     Note
     The contrast threshold will be divided by nOctaveLayers when the filtering is applied. When nOctaveLayers is set to default and if you want to use the value used in D. Lowe paper, 0.03, set this argument to 0.09.
     Parameters
     edgeThreshold    The threshold used to filter out edge-like features. Note that the its meaning is different from the contrastThreshold, i.e. the larger the edgeThreshold, the less features are filtered out (more features are retained).
     sigma    The sigma of the Gaussian applied to the input image at the octave #0. If your image is captured with a weak camera with soft lenses, you might want to reduce the number.
     )
     */
    SIFTOpenCV() : f2d(//cv::SIFT::create(0, 3, 0.04, 10, 1.6, CV_32F) // <-- CV_32F doesn't work -- gives `libc++abi.dylib: terminating with uncaught exception of type cv::Exception: OpenCV(4.5.2) /tmp/nix-build-opencv-4.5.2.drv-0/source/modules/features2d/src/sift.dispatch.cpp:477: error: (-5:Bad argument) image is empty or has incorrect depth (!=CV_8U) in function 'detectAndCompute'`
                       //cv::SIFT::create()
                       cv::SIFT::create(nfeatures,
                                        nOctaveLayers,
                                        contrastThreshold,
                                        edgeThreshold,
                                        sigma)) {
    }
    
    std::pair<std::vector<cv::KeyPoint>, cv::Mat /*descriptors*/> findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale);
    
    void findHomography(ProcessedImage<SIFTOpenCV>& img1, ProcessedImage<SIFTOpenCV>& img2, SIFTState& s
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
#endif
                        );
    
protected:
    std::vector<cv::KeyPoint> detect(cv::Mat& img) {
        //-- Step 1: Detect the keypoints:
        std::vector<cv::KeyPoint> keypoints;
        f2d->detect( img, keypoints );
        return keypoints;
    }
    
    // TODO: unused function
    cv::Mat descriptors(cv::Mat& img, std::vector<cv::KeyPoint>& keypoints) {
        //-- Step 2: Calculate descriptors (feature vectors)
        cv::Mat descriptors;
        f2d->compute( img, keypoints, descriptors );
        return descriptors;
    }
    
    // TODO: unused function
    std::vector<cv::DMatch> match(cv::Mat& descriptors_1, cv::Mat& descriptors_2) {
        //-- Step 3: Matching descriptor vectors using BFMatcher :
        cv::BFMatcher matcher;
        std::vector< cv::DMatch > matches;
        matcher.match( descriptors_1, descriptors_2, matches );
        return matches;
    }
    
    cv::Ptr<cv::Feature2D> f2d;
};
/* https://stackoverflow.com/questions/27533203/how-do-i-use-sift-in-opencv-3-0-with-c :
 #include "opencv2/xfeatures2d.hpp"

 //
 // now, you can no more create an instance on the 'stack', like in the tutorial
 // (yea, noticed for a fix/pr).
 // you will have to use cv::Ptr all the way down:
 //
 cv::Ptr<Feature2D> f2d = xfeatures2d::SIFT::create();
 //cv::Ptr<Feature2D> f2d = xfeatures2d::SURF::create();
 //cv::Ptr<Feature2D> f2d = ORB::create();
 // you get the picture, i hope..

 //-- Step 1: Detect the keypoints:
 std::vector<KeyPoint> keypoints_1, keypoints_2;
 f2d->detect( img_1, keypoints_1 );
 f2d->detect( img_2, keypoints_2 );

 //-- Step 2: Calculate descriptors (feature vectors)
 Mat descriptors_1, descriptors_2;
 f2d->compute( img_1, keypoints_1, descriptors_1 );
 f2d->compute( img_2, keypoints_2, descriptors_2 );

 //-- Step 3: Matching descriptor vectors using BFMatcher :
 BFMatcher matcher;
 std::vector< DMatch > matches;
 matcher.match( descriptors_1, descriptors_2, matches );
 */

#elif defined(SIFTGPU_)
struct SIFTGPU : public SIFTBase {
    std::pair<std::vector<SiftGPU::SiftKeypoint> /*keys1*/, std::pair<std::vector<float> /*descriptors1*/, int /*num1*/>> findKeypoints(int threadID, SIFTState& s, SIFTParams& p, cv::Mat& greyscale);

    // Finds matching and homography
    void findHomography(ProcessedImage<SIFTGPU>& img1, ProcessedImage<SIFTGPU>& img2, SIFTState& s
    #ifdef USE_COMMAND_LINE_ARGS
        , DataSourceBase* src, CommandLineConfig& cfg
    #endif
    );

#ifdef SIFTGPU_TEST
    static int test(int argc, char** argv);
#endif
};
#endif


// Idea here: done: Use this sometime to wrap all SIFT implementations you can choose from: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

#endif /* SIFT_hpp */
