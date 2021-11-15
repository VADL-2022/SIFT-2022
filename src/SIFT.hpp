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

template <typename SIFT_Type>
struct ProcessedImage {};

// Forward declarations //
struct SIFTAnatomy;
struct SIFTOpenCV;
// //

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
};

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
    void resetMatching() {}
    // //
};


struct SIFTBase {
    
};

struct SIFTAnatomy : public SIFTBase {
    SIFTAnatomy() {
        
    }
    
    std::pair<sift_keypoints* /*keypoints*/, std::pair<sift_keypoint_std* /*k*/, int /*n*/>> findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale);
    
    // Finds matching and homography
    void findHomography(ProcessedImage<SIFTAnatomy>& img1, ProcessedImage<SIFTAnatomy>& img2);
};

struct SIFTOpenCV : public SIFTBase {
    SIFTOpenCV() : f2d(cv::SIFT::create()) {
    }
    
    std::vector<cv::KeyPoint> findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale);
    
    void findHomography(ProcessedImage<SIFTOpenCV>& img1, ProcessedImage<SIFTOpenCV>& img2);
    
protected:
    std::vector<cv::KeyPoint> detect(cv::Mat img) {
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


// Idea here: TODO: Use this sometime to wrap all SIFT implementations you can choose from: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

#endif /* SIFT_hpp */
