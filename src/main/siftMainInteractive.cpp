//
//  siftMainInteractive.cpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "siftMainInteractive.hpp"
#include "siftMainCmdConfig.hpp"

#include "../Config.hpp"
#include "../DataOutput.hpp"
#include "../KeypointsAndMatching.hpp"
#include "../compareKeypoints.hpp"

#ifdef USE_COMMAND_LINE_ARGS
#ifdef SIFTAnatomy_
int mainInteractive(DataSourceBase* src,
                    SIFTState& s,
                    SIFTParams& p,
                    size_t skip,
                    DataOutputBase& o
                    #ifdef USE_COMMAND_LINE_ARGS
                      , FileDataOutput& o2,
                      const CommandLineConfig& cfg
                    #endif
) {
    cv::Mat canvas;
    
	//--- print the files sorted by filename
    bool retryNeeded = false;
    for (size_t i = src->currentIndex;; i++) {
        std::cout << "i: " << i << std::endl;
        cv::Mat mat = src->get(i);
        if (mat.empty()) { printf("No more images left to process. Exiting.\n"); break; }
        cv::Mat greyscale = src->siftImageForMat(i);
        float* x = (float*)greyscale.data;
        size_t w = mat.cols, h = mat.rows;
        auto path = src->nameForIndex(i);

        // Initialize canvas if needed
        if (canvas.data == nullptr) {
            puts("Init canvas");
            canvas = cv::Mat(h, w, CV_32FC4);
        }

		// compute sift keypoints
		int n; // Number of keypoints
		struct sift_keypoints* keypoints;
		struct sift_keypoint_std *k = nullptr;
        if (!CMD_CONFIG(imageCaptureOnly)) {
            if (s.loadedKeypoints == nullptr) {
                t.reset();
                k = my_sift_compute_features(p.params, x, w, h, &n, &keypoints);
                printf("Number of keypoints: %d\n", n);
                t.logElapsed("compute features");
            }
            else {
                k = s.loadedK.release(); // "Releases the ownership of the managed object if any." ( https://en.cppreference.com/w/cpp/memory/unique_ptr/release )
                keypoints = s.loadedKeypoints.release();
                n = s.loadedKeypointsSize;
            }
        }

        cv::Mat backtorgb = src->colorImageForMat(i);
		if (i == skip) {
			puts("Init firstImage");
            s.firstImage = backtorgb;
		}

		// Draw keypoints on `o.canvas`
        if (!CMD_CONFIG(imageCaptureOnly)) {
            t.reset();
        }
        backtorgb.copyTo(canvas);
        if (!CMD_CONFIG(imageCaptureOnly)) {
            for(int i=0; i<n; i++){
                drawSquare(canvas, cv::Point(k[i].x, k[i].y), k[i].scale, k[i].orientation, 1);
                //break;
                // fprintf(f, "%f %f %f %f ", k[i].x, k[i].y, k[i].scale, k[i].orientation);
                // for(int j=0; j<128; j++){
                // 	fprintf(f, "%u ", k[i].descriptor[j]);
                // }
                // fprintf(f, "\n");
            }
            t.logElapsed("draw keypoints");
        }

		// Compare keypoints if we had some previously and render to canvas if needed
        if (!CMD_CONFIG(imageCaptureOnly)) {
            retryNeeded = compareKeypoints(canvas, s, p, keypoints, backtorgb COMPARE_KEYPOINTS_ADDITIONAL_ARGS);
        }

        bool exit;
        if (CMD_CONFIG(imageFileOutput)) {
            #ifdef USE_COMMAND_LINE_ARGS
            exit = run(o2, canvas, *src, s, p, backtorgb, keypoints, retryNeeded, i, n);
            #endif
        }
        else {
            exit = run(o, canvas, *src, s, p, backtorgb, keypoints, retryNeeded, i, n);
        }
	
		// write to standard output
		//sift_write_to_file("/dev/stdout", k, n);

		// cleanup
        free(k);
		
		// Save keypoints
        if (!retryNeeded) {
            s.computedKeypoints.push_back(keypoints);
        }
        
        if (exit) {
            break;
        }
		
		// Reset RNG so some colors coincide
		resetRNG();
	}
    
    if (CMD_CONFIG(imageFileOutput)) {
        #ifdef USE_COMMAND_LINE_ARGS
        o2.release(); // Save the file
        #endif
    }
    
    return 0;
}
#endif
#endif // USE_COMMAND_LINE_ARGS
