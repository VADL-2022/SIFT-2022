//
//  compareKeypoints.cpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "compareKeypoints.hpp"

#include "KeypointsAndMatching.hpp"

bool compareKeypoints(PreviewWindowDataOutput& o, SIFTState& s, SIFTParams& p, struct sift_keypoints* keypoints, cv::Mat& backtorgb) {
    bool retval = false;
    
    // Compare keypoints if we had some previously
    if (s.computedKeypoints.size() > 0) {
        struct sift_keypoints* keypointsPrev = s.computedKeypoints.back();
        if (s.loaded_out_k1) {
            s.out_k1 = s.loaded_out_k1.release();
            s.out_k2A = s.loaded_out_k2A.release();
            s.out_k2B = s.loaded_out_k2B.release();
            
            // Setting loaded parameters for matching
            p.meth_flag = p.loaded_meth_flag;
            p.thresh = p.loaded_thresh;
        }
        else {
            s.out_k1 = sift_malloc_keypoints();
            s.out_k2A = sift_malloc_keypoints();
            s.out_k2B = sift_malloc_keypoints();
            
            // Setting current parameters for matching
            p.meth_flag = 1;
            p.thresh = 0.6;

            // Matching
            t.reset();
            matching(keypointsPrev, keypoints, s.out_k1, s.out_k2A, s.out_k2B, p.thresh, p.meth_flag);
            t.logElapsed("find matches");
        }

        // Draw matches
        t.reset();
        struct sift_keypoints* k1 = s.out_k1;
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

                drawSquare(backtorgb, cv::Point(s.out_k2A->list[i]->x, s.out_k2A->list[i]->y), s.out_k2A->list[i]->sigma /* need to choose something better here */, s.out_k2A->list[i]->theta, 2);
                cv::line(backtorgb, cv::Point(s.out_k1->list[i]->x, s.out_k1->list[i]->y), cv::Point(s.out_k2A->list[i]->x, s.out_k2A->list[i]->y), lastColor, 1);
            }
        }
        t.logElapsed("draw matches");

        t.reset();
        // Find the homography matrix between the previous and current image ( https://docs.opencv.org/4.5.2/d1/de0/tutorial_py_feature_homography.html )
        //const int MIN_MATCH_COUNT = 10
        // https://docs.opencv.org/3.4/d7/dff/tutorial_feature_homography.html
        std::vector<cv::Point2f> obj; // The `obj` is the current image's keypoints
        std::vector<cv::Point2f> scene; // The `scene` is the previous image's keypoints. "Scene" means the area within which we are finding `obj` and this is since we want to find the previous image in the current image since we're falling down and therefore "zooming in" so the current image should be within the previous one.
        obj.reserve(k1->size);
        scene.reserve(k1->size);
        // TODO: reuse memory of k1 instead of copying?
        for (size_t i = 0; i < k1->size; i++) {
            obj.emplace_back(s.out_k2A->list[i]->x, s.out_k2A->list[i]->y);
            scene.emplace_back(s.out_k1->list[i]->x, s.out_k1->list[i]->y);
        }
        
        // Make a matrix in transformations history
        if (obj.size() < 4 || scene.size() < 4) { // Prevents "libc++abi.dylib: terminating with uncaught exception of type cv::Exception: OpenCV(4.5.2) /tmp/nix-build-opencv-4.5.2.drv-0/source/modules/calib3d/src/fundam.cpp:385: error: (-28:Unknown error code -28) The input arrays should have at least 4 corresponding point sets to calculate Homography in function 'findHomography'"
            printf("Not enough keypoints to find homography! Trying to find keypoints on previous image again with tweaked params\n");
            // Retry with tweaked params
            p.params->n_oct++;
            p.params->n_bins++;
            p.params->n_hist++;
            if (p.params->n_spo < 2) {
                p.params->n_spo = 2;
            }
            p.params->sigma_min -= 0.05;
            p.params->delta_min -= 0.05;
            p.params->sigma_in -= 0.05;
            
            // Go back to previous file as well
            retval = true;
        }
        if (!retval) {
            s.allTransformations.emplace_back(cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ ));
            t.logElapsed("find homography");
        }
        
        t.reset();
        // Save to canvas
        cv::Mat& img_matches = backtorgb; // The image on which to draw the lines showing corners of the object (current image)
        img_matches.copyTo(o.canvas);
        t.logElapsed("render to canvas: with prev keypoints");
        
        // Cleanup //
        if (!retval) {
            sift_free_keypoints(keypointsPrev);
            s.computedKeypoints.pop_back();
        }
        // //
    }
    else {
        t.reset();
        // Save to canvas
        backtorgb.copyTo(o.canvas);
        t.logElapsed("render to canvas: no prev keypoints");
    }
    
    return retval;
}
