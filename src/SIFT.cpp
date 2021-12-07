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

#ifdef USE_COMMAND_LINE_ARGS
#include "DataSource.hpp"
#endif

std::pair<sift_keypoints* /*keypoints*/, std::pair<sift_keypoint_std* /*k*/, int /*n*/>> SIFTAnatomy::findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale) {
    assert(greyscale.depth() == CV_32F);
    assert(greyscale.type() == CV_32FC1);
    float* x = (float*)greyscale.data;
    size_t w = greyscale.cols, h = greyscale.rows;
    std::cout << "width: " << w << ", height: " << h << std::endl;

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
        //stopMain();
        
//                t.logElapsed(id, "compute features");
//                goto end;
    }
    t.logElapsed(threadID, "compute features");

    return std::make_pair(keypoints, std::make_pair(k, n));
}

// Finds matching and homography
void SIFTAnatomy::findHomography(ProcessedImage<SIFTAnatomy>& img1, ProcessedImage<SIFTAnatomy>& img2
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
#endif
) {
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

    if (CMD_CONFIG(mainMission)) {
#ifdef USE_COMMAND_LINE_ARGS
        // Draw it //
        cv::Mat backtorgb = src->colorImageForMat(img2.i);
        backtorgb.copyTo(img2.canvas);
        
        // Draw keypoints on `img2.canvas`
        struct sift_keypoint_std *k = img2.k.get();
        int n = img2.n;
        t.reset();
        for(int i=0; i<n; i++){
            drawSquare(img2.canvas, cv::Point(k[i].x, k[i].y), k[i].scale, k[i].orientation, 1);
            //break;
            // fprintf(f, "%f %f %f %f ", k[i].x, k[i].y, k[i].scale, k[i].orientation);
            // for(int j=0; j<128; j++){
            //     fprintf(f, "%u ", k[i].descriptor[j]);
            // }
            // fprintf(f, "\n");
        }
        t.logElapsed("draw keypoints");
        
        // Draw matches
        t.reset();
        auto& s = img2;
        struct sift_keypoints* k1 = s.out_k1.get();
        printf("Number of matching keypoints: %d\n", k1->size);
        if (k1->size > 0){

            int n_hist = k1->list[0]->n_hist;
            int n_ori = k1->list[0]->n_ori;
            int dim = n_hist*n_hist*n_ori;
            int n_bins  = k1->list[0]->n_bins;
            int n = k1->size;
            for(int i = 0; i < n; i++){
                // fprintf_one_keypoint(f, k1->list[i], dim, n_bins, 2);
                // fprintf_one_keypoint(f, k2A->list[i], dim, n_bins, 2);
                // fprintf_one_keypoint(f, k2B->list[i], dim, n_bins, 2);
                // fprintf(f, "\n");

		    auto xoff= 100;
		    auto yoff=-100;
                drawSquare(img2.canvas, cv::Point(s.out_k2A->list[i]->x+xoff, s.out_k2A->list[i]->y+yoff), s.out_k2A->list[i]->sigma /* need to choose something better here */, s.out_k2A->list[i]->theta, 2);
                cv::line(img2.canvas, cv::Point(s.out_k1->list[i]->x+xoff, s.out_k1->list[i]->y+yoff), cv::Point(s.out_k2A->list[i]->x+xoff, s.out_k2A->list[i]->y+yoff), lastColor, 1);
            }
        }
        t.logElapsed("draw matches");
        
        t.reset();
        // Reset RNG so some colors coincide
        resetRNG();
        t.logElapsed("reset RNG");
        // //
#endif
    }
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
    
    printf("Thread %d: Number of keypoints: %zu\n", threadID, keypoints.size());
    if (keypoints.size() < 4) {
        printf("Not enough keypoints to find homography! Ignoring this image\n");
        // TODO: Simply let the transformation be an identity matrix?
        //exit(3);
    }
    t.logElapsed(threadID, "compute features");
    
    return std::make_pair(keypoints, descriptors);
}

void SIFTOpenCV::findHomography(ProcessedImage<SIFTOpenCV>& img1, ProcessedImage<SIFTOpenCV>& img2
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
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
        // Make it fill the screen
        cv::Mat img1Resized, img2Resized;
        cv::resize(img1.image, img1Resized, g_desktopSize.size());
        cv::resize(img2.image, img2Resized, g_desktopSize.size());
        //if (!img2.canvas.empty()) {
            // Get scale to destination
            cv::Size2d scale = cv::Size2d((double)g_desktopSize.size().width / img2.image.cols, (double)g_desktopSize.size().height / img2.image.rows);
            std::cout << "Scale: " << scale << std::endl;
            float max = std::max(scale.width, scale.height);
            
            // Resize canvas
            //cv::resize(img2.canvas, img2.canvas, g_desktopSize.size(), 0, 0, cv::INTER_LINEAR);
            // Scale all keypoints
            auto s = [&](std::vector<cv::KeyPoint>& computedKeypoints) {
                for (cv::KeyPoint& p : computedKeypoints) {
                    p.pt.x *= scale.width;
                    p.pt.y *= scale.height;
                    p.size *= max;
                }
            };
            s(img1.computedKeypoints);
            s(img2.computedKeypoints);
        //}
        
        // Draw it
        // Side-by-side matches: //
//        cv::drawMatches(img1Resized, img1.computedKeypoints, img2Resized, img2.computedKeypoints, img2.matches, img2.canvas);
        // //
        // Single image //
        img2Resized.copyTo(img2.canvas);
        for (auto& match : img2.matches) {
            cv::KeyPoint& p1 = img1.computedKeypoints[match.trainIdx];
            cv::KeyPoint& p2 = img2.computedKeypoints[match.queryIdx];

            //const int draw_shift_bits = 4;
            //const int draw_multiplier = 1 << draw_shift_bits;
            cv::Point center1( cvRound(p1.pt.x), cvRound(p1.pt.y) ); // TODO: FIXME: With address sanitizer, this says "Heap buffer overflow"
            cv::Point center2( cvRound(p2.pt.x), cvRound(p2.pt.y) );
            drawCircle(img2.canvas, center1, 2);
            drawCircle(img2.canvas, center2, 2);
            cv::line(img2.canvas, center1, center2, lastColor, 1);
        }
        // //
#endif
    }
}
