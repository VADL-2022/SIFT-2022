//
//  siftMain.cpp
//  (Originally example2.cpp)
//  SIFT
//
//  Created by VADL on 9/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//
// This file runs on the SIFT RPis. It can also run on your computer. You can optionally show a preview window for fine-grain control.

#include "common.hpp"

#include "KeypointsAndMatching.hpp"
#include "compareKeypoints.hpp"
#include "DataSource.hpp"
#include "DataOutput.hpp"

// https://docs.opencv.org/master/da/d6a/tutorial_trackbar.html
const int alpha_slider_max = 2;
int alpha_slider = 0;
double alpha;
double beta;
static void on_trackbar( int, void* )
{
   alpha = (double) alpha_slider/alpha_slider_max ;
}

// Config //
// Data source
//using DataSourceT = FolderDataSource;
using DataSourceT = CameraDataSource;

// Data output
using DataOutputT = PreviewWindowDataOutput;
// //
int main(int argc, char **argv)
{
	// Set the default "skip"
    size_t skip = 0;//120;//60;//100;//38;//0;
    DataSourceT src = makeDataSource<DataSourceT>(argc, argv, skip); // Read folder determined by command-line arguments
    DataOutputT o;
    bool imageCaptureOnly = false, imageFileOutput = false;
    FileDataOutput o2;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image-capture-only") == 0) { // For not running SIFT
            imageCaptureOnly = true;
        }
        else if (strcmp(argv[i], "--image-file-output")) { // Outputs to video instead of preview window
            imageFileOutput = true;
        }
    }
	
	//--- print the files sorted by filename
    SIFTState s;
    SIFTParams p;
    bool retryNeeded = false;
    for (size_t i = src.currentIndex;; i++) {
        std::cout << "i: " << i << std::endl;
        cv::Mat mat = src.get(i);
        cv::Mat greyscale = src.siftImageForMat(i);
        float* x = (float*)greyscale.data;
        size_t w = mat.cols, h = mat.rows;
        auto path = src.nameForIndex(i);

		// Initialize canvas if needed
        o.init(w, h);

		// compute sift keypoints
		int n; // Number of keypoints
		struct sift_keypoints* keypoints;
		struct sift_keypoint_std *k;
        if (!imageCaptureOnly) {
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

        cv::Mat backtorgb = src.colorImageForMat(i);
		if (i == skip) {
			puts("Init firstImage");
            s.firstImage = backtorgb;
		}

		// Draw keypoints on `o.canvas`
        if (!imageCaptureOnly) {
            t.reset();
        }
        backtorgb.copyTo(o.canvas);
        if (!imageCaptureOnly) {
            for(int i=0; i<n; i++){
                drawSquare(o.canvas, cv::Point(k[i].x, k[i].y), k[i].scale, k[i].orientation, 1);
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
        if (!imageCaptureOnly) {
            retryNeeded = compareKeypoints(o, s, p, keypoints, backtorgb);
        }

        if (imageFileOutput) {
            run(o2, src, s, p, backtorgb, keypoints, retryNeeded, i, n);
        }
        else {
            run(o, src, s, p, backtorgb, keypoints, retryNeeded, i, n);
        }
	
		// write to standard output
		//sift_write_to_file("/dev/stdout", k, n);

		// cleanup
        free(k);
		
		// Save keypoints
        if (!retryNeeded) {
            s.computedKeypoints.push_back(keypoints);
        }
		
		// Reset RNG so some colors coincide
		resetRNG();
	}
	
	return 0;
}
