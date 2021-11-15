//
//  SIFT.cpp
//  SIFT
//
//  Created by VADL on 11/14/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "SIFT.hpp"

#include "Config.hpp"

#include "siftMain.hpp"

std::pair<sift_keypoints* /*keypoints*/, std::pair<sift_keypoint_std* /*k*/, int /*n*/>> SIFTAnatomy::findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale) {
    float* x = (float*)greyscale.data;
    size_t w = greyscale.cols, h = greyscale.rows;

    // compute sift keypoints
    t.reset();
    int n; // Number of keypoints
    struct sift_keypoints* keypoints;
    struct sift_keypoint_std *k;
    k = my_sift_compute_features(p.params, x, w, h, &n, &keypoints);
    printf("Thread %d: Number of keypoints: %d\n", threadID, n);
    if (n < 4) {
        printf("Not enough keypoints to find homography! Ignoring this image\n");
        // TODO: Simply let the transformation be an identity matrix?
        //exit(3);
	tp.isStop = true;
        
//                t.logElapsed(id, "compute features");
//                goto end;
    }
    t.logElapsed(threadID, "compute features");

    return std::make_pair(keypoints, std::make_pair(k, n));
}

// Finds matching and homography
void SIFTAnatomy::findHomography(ProcessedImage<SIFTAnatomy>& img1, ProcessedImage<SIFTAnatomy>& img2) {
    struct sift_keypoints* keypointsPrev = img1.computedKeypoints.get();
    struct sift_keypoints* keypoints = img2.computedKeypoints.get();
    std::cout << keypointsPrev << ", " << keypoints << std::endl;
    assert(keypointsPrev != nullptr && keypoints != nullptr); // This should always hold, since the other thread pool threads enqueued them.
    assert(img2.out_k1 == nullptr && img2.out_k2A == nullptr && img2.out_k2B == nullptr); // This should always hold since we reset the pointers when done with matching
    img2.out_k1.reset(sift_malloc_keypoints());
    img2.out_k2A.reset(sift_malloc_keypoints());
    img2.out_k2B.reset(sift_malloc_keypoints());
    if (keypointsPrev->size == 0 || keypoints->size == 0) {
        std::cout << "Matcher thread: zero keypoints, cannot match" << std::endl;
        throw ""; // TODO: temp, need to notify main thread and retry matching maybe
    }
    matching(keypointsPrev, keypoints, img2.out_k1.get(), img2.out_k2A.get(), img2.out_k2B.get(), img2.p.thresh, img2.p.meth_flag);
    t.logElapsed("Matcher thread: find matches");
    
    // Find homography
    t.reset();
    struct sift_keypoints* k1 = img2.out_k1.get();
    struct sift_keypoints* out_k2A = img2.out_k2A.get();
    struct sift_keypoints* out_k1 = k1;
    // Find the homography matrix between the previous and current image ( https://docs.opencv.org/4.5.2/d1/de0/tutorial_py_feature_homography.html )
    //const int MIN_MATCH_COUNT = 10
    // https://docs.opencv.org/3.4/d7/dff/tutorial_feature_homography.html
    std::vector<cv::Point2f> obj; // The `obj` is the current image's keypoints
    std::vector<cv::Point2f> scene; // The `scene` is the previous image's keypoints. "Scene" means the area within which we are finding `obj` and this is since we want to find the previous image in the current image since we're falling down and therefore "zooming in" so the current image should be within the previous one.
    obj.reserve(k1->size);
    scene.reserve(k1->size);
    // TODO: reuse memory of k1 instead of copying?
    for (size_t i = 0; i < k1->size; i++) {
        obj.emplace_back(out_k2A->list[i]->x, out_k2A->list[i]->y);
        scene.emplace_back(out_k1->list[i]->x, out_k1->list[i]->y);
    }
    
    // Make a matrix in transformations history
    if (obj.size() < 4 || scene.size() < 4) { // Prevents "libc++abi.dylib: terminating with uncaught exception of type cv::Exception: OpenCV(4.5.2) /tmp/nix-build-opencv-4.5.2.drv-0/source/modules/calib3d/src/fundam.cpp:385: error: (-28:Unknown error code -28) The input arrays should have at least 4 corresponding point sets to calculate Homography in function 'findHomography'"
        //printf("Not enough keypoints to find homography! Trying to find keypoints on previous image again with tweaked params\n");
//            // Retry with tweaked params
//            img2.p.params->n_oct++;
//            img2.p.params->n_bins++;
//            img2.p.params->n_hist++;
//            if (img2.p.params->n_spo < 2) {
//                img2.p.params->n_spo = 2;
//            }
//            img2.p.params->sigma_min -= 0.05;
//            img2.p.params->delta_min -= 0.05;
//            img2.p.params->sigma_in -= 0.05;
        
        //throw "Not yet implemented"; // This is way too complex..
        
        
        //printf("Not enough keypoints to find homography! Ignoring this image\n");
        //goto end;
        
        printf("Not enough keypoints to find homography! Exiting..");
        exit(3);
    }
    
    img2.transformation = cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ );
}

std::pair<std::vector<cv::KeyPoint>, cv::Mat /*descriptors*/> SIFTOpenCV::findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale) {
//    t.reset();
//    auto ret = detect(greyscale);
//    t.logElapsed(threadID, "compute features");
//    t.reset();
//    auto ret2 = descriptors(greyscale, ret);
//    t.logElapsed(threadID, "compute descriptors");
//    return std::make_pair(ret, ret2);

    t.reset();
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    f2d->detectAndCompute(greyscale, cv::Mat(), keypoints, descriptors);
    
    printf("Thread %d: Number of keypoints: %d\n", threadID, keypoints.size());
    if (keypoints.size() < 4) {
        printf("Not enough keypoints to find homography! Ignoring this image\n");
        // TODO: Simply let the transformation be an identity matrix?
        exit(3);
    }
    t.logElapsed(threadID, "compute features");
    
    return std::make_pair(keypoints, descriptors);
}

void SIFTOpenCV::findHomography(ProcessedImage<SIFTOpenCV>& img1, ProcessedImage<SIFTOpenCV>& img2
#ifdef USE_COMMAND_LINE_ARGS
    , cv::Mat canvas, CommandLineConfig& cfg
#endif
) {
    img2.matches = match(img1.descriptors, img2.descriptors);
    
    // Using descriptors (makes it able to match each feature using scale invariance):
    // https://stackoverflow.com/questions/13318853/opencv-drawmatches-queryidx-and-trainidx , https://stackoverflow.com/questions/30716610/how-to-get-pixel-coordinates-from-feature-matching-in-opencv-python
    std::vector<cv::Point2f> obj; obj.reserve(img2.matches.size());
    std::vector<cv::Point2f> scene; obj.reserve(img1.matches.size()); // TODO: correct order?
    for (cv::DMatch match : img2.matches) {
        scene.push_back(img1.computedKeypoints[match.queryIdx].pt);
        obj.push_back(img2.computedKeypoints[match.trainIdx].pt);
    }
    
    // Basic:
//    std::vector<cv::Point2f> obj;
//    std::vector<cv::Point2f> scene;
//    cv::KeyPoint::convert(img2.computedKeypoints, obj);
//    cv::KeyPoint::convert(img1.computedKeypoints, scene); // TODO: correct order?
//    img2.transformation = cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ );
    
    // Better:
    img2.transformation = cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ );
    
    if (CMD_CONFIG(mainMission)) {
#ifdef USE_COMMAND_LINE_ARGS
        // Draw it
        cv::drawMatches(img1.image, img1.computedKeypoints, img2.image, img2.computedKeypoints, img2.matches, img2.canvas);
#endif
    }
}
