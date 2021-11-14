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
#include "utils.hpp"

#ifndef USE_COMMAND_LINE_ARGS
#include "Queue.hpp"
#include "lib/ctpl_stl.hpp"
struct ProcessedImage {
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
    // //
    
    SIFTParams p;
    size_t i;
};
Queue<ProcessedImage, 256 /*32*/> processedImageQueue;
cv::Mat lastImageToFirstImageTransformation; // "Message box" for the accumulated transformations so far
#endif

//// https://docs.opencv.org/master/da/d6a/tutorial_trackbar.html
//const int alpha_slider_max = 2;
//int alpha_slider = 0;
//double alpha;
//double beta;
//static void on_trackbar( int, void* )
//{
//   alpha = (double) alpha_slider/alpha_slider_max ;
//}

// Config //
// Data source
//using DataSourceT = FolderDataSource;
using DataSourceT = CameraDataSource;

// Data output
using DataOutputT = PreviewWindowDataOutput;
// //

template <typename DataSourceT, typename DataOutputT>
int mainInteractive(DataSourceT* src,
                    SIFTState& s,
                    SIFTParams& p,
                    size_t skip,
                    DataOutputT& o
                    #ifdef USE_COMMAND_LINE_ARGS
                      , FileDataOutput& o2,
                      const CommandLineConfig& cfg
                    #endif
);

template <typename DataSourceT, typename DataOutputT>
int mainMission(DataSourceT* src,
                SIFTParams& p,
                DataOutputT& o
                );

int main(int argc, char **argv)
{
    SIFTState s;
    SIFTParams p;
    
	// Set the default "skip"
    size_t skip = 0;//120;//60;//100;//38;//0;
    DataOutputT o;
    
    // Command-line args //
#ifdef USE_COMMAND_LINE_ARGS
    p.params = sift_assign_default_parameters();
    CommandLineConfig cfg;
    FileDataOutput o2("dataOutput/live", 1.0 /* fps */ /*, sizeFrame */);
    std::unique_ptr<DataSourceBase> src;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--folder-data-source") == 0) { // Get from folder instead of camera
            src = std::make_unique<FolderDataSource>(argc, argv, skip); // Read folder determined by command-line arguments
            cfg.folderDataSource = true;
        }
        else if (strcmp(argv[i], "--image-capture-only") == 0) { // For not running SIFT
            cfg.imageCaptureOnly = true;
        }
        else if (strcmp(argv[i], "--image-file-output") == 0) { // Outputs to video instead of preview window
            cfg.imageFileOutput = true;
        }
        else if (i+1 < argc && strcmp(argv[i], "--sift-params") == 0) { // Outputs to video instead of preview window
            cfg.imageFileOutput = true;
            if (SIFTParams::call_params_function(argv[i+1], p.params) < 0) {
                printf("Invalid params function name given: %s. Value options are:", argv[i+1]);
                SIFTParams::print_params_functions();
                printf(". Exiting.\n");
                exit(1);
            }
            else {
                printf("Using params function %s\n", argv[i+1]);
            }
            i++;
        }
    }
    
    if (!cfg.folderDataSource) {
        src = std::make_unique<DataSourceT>();
    }
    
    mainInteractive(src.get(), s, p, skip, o, o2, cfg);
#else
    // Set params (will use default if none set) //
    //USE(v3Params);
    //USE(v2Params);
    // //
    
    DataSourceT src_ = makeDataSource<DataSourceT>(argc, argv, skip); // Read folder determined by command-line arguments
    DataSourceT* src = &src_;
    
    mainMission(src, p, o);
#endif
    // //
    
	return 0;
}

#ifndef USE_COMMAND_LINE_ARGS
// https://stackoverflow.com/questions/2808398/easily-measure-elapsed-time
template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

void* matcherThreadFunc(void* arg) {
    std::atomic<bool>& isStop = *(std::atomic<bool>*)arg;
    do {
        ProcessedImage img1, img2;
        std::cout << "Matcher thread: Locking for dequeueOnceOnTwoImages" << std::endl;
        pthread_mutex_lock( &processedImageQueue.mutex );
        while( processedImageQueue.count <= 1 ) // Wait until not empty.
        {
            pthread_cond_wait( &processedImageQueue.condition, &processedImageQueue.mutex );
            std::cout << "Matcher thread: Unlocking for dequeueOnceOnTwoImages 2" << std::endl;
            pthread_mutex_unlock( &processedImageQueue.mutex );
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Matcher thread: Locking for dequeueOnceOnTwoImages 2" << std::endl;
            pthread_mutex_lock( &processedImageQueue.mutex );
        }
        std::cout << "Matcher thread: Unlocking for dequeueOnceOnTwoImages 3" << std::endl;
        pthread_mutex_unlock( &processedImageQueue.mutex );
        std::cout << "Matcher thread: Locking for dequeueOnceOnTwoImages 3" << std::endl;
        processedImageQueue.dequeueOnceOnTwoImages(&img1, &img2);
        std::cout << "Matcher thread: Unlocked for dequeueOnceOnTwoImages" << std::endl;
        
        // Setting current parameters for matching
        img2.p.meth_flag = 1;
        img2.p.thresh = 0.6;

        // Matching
        t.reset();
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
            
            
            printf("Not enough keypoints to find homography! Ignoring this image\n");
            throw "";
            continue;
        }
        
        img2.transformation = cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ );
        // Accumulate homography
        if (lastImageToFirstImageTransformation.empty()) {
            lastImageToFirstImageTransformation = img2.transformation.inv();
        }
        else {
            lastImageToFirstImageTransformation *= img2.transformation.inv();
        }
        t.logElapsed("find homography");
        
        // Free memory and mark this as an unused ProcessedImage:
        img2.out_k1.reset();
        img2.out_k2A.reset();
        img2.out_k2B.reset();
        
    } while (!isStop);
    return (void*)0;
}

template <typename DataSourceT, typename DataOutputT>
int mainMission(DataSourceT* src,
                SIFTParams& p,
                DataOutputT& o
) {
    cv::Mat firstImage;
    cv::Mat prevImage;
    ctpl::thread_pool tp(4); // Number of threads in the pool
    // ^^ Note: "the destructor waits for all the functions in the queue to be finished" (or call .stop())
    pthread_t matcherThread;
    pthread_create(&matcherThread, NULL, matcherThreadFunc, &tp.isStop);
    
    auto last = std::chrono::steady_clock::now();
    auto fps = src->fps();
    const long long timeBetweenFrames_milliseconds = 1/fps * 1000;
    std::cout << "Target fps: " << fps << std::endl;
    std::atomic<size_t> offset = 0; // Moves back the indices shown to SIFT threads
    for (size_t i = src->currentIndex;; i++) {
        std::cout << "i: " << i << std::endl;
        if (src->wantsCustomFPS()) {
            auto sinceLast_milliseconds = since(last).count();
            if (sinceLast_milliseconds < timeBetweenFrames_milliseconds) {
                // Sleep
                auto sleepTime = timeBetweenFrames_milliseconds - sinceLast_milliseconds;
                std::cout << "Sleeping for " << sleepTime << " milliseconds" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
            last = std::chrono::steady_clock::now();
        }
        t.reset();
        cv::Mat mat = src->get(i);
        std::cout << "CAP_PROP_POS_MSEC: " << src->timeMilliseconds() << std::endl;
        t.logElapsed("get image");
        if (mat.empty()) { printf("No more images left to process. Exiting.\n"); break; }
        t.reset();
        cv::Mat greyscale = src->siftImageForMat(i);
        t.logElapsed("siftImageForMat");
        float* x = (float*)greyscale.data;
        size_t w = mat.cols, h = mat.rows;
        //auto path = src->nameForIndex(i);
        
        tp.push([&offset, &pOrig=p, x, w, h](int id, /*extra args:*/ size_t i, cv::Mat mat, cv::Mat prevImage) {
            std::cout << "hello from " << id << std::endl;
            
            SIFTParams p(pOrig); // New version of the params we can modify (separately from the other threads)
            p.params = sift_assign_default_parameters();

            // compute sift keypoints
            t.reset();
            int n; // Number of keypoints
            struct sift_keypoints* keypoints;
            struct sift_keypoint_std *k;
            k = my_sift_compute_features(p.params, x, w, h, &n, &keypoints);
            printf("Thread %d: Number of keypoints: %d\n", id, n);
            if (n < 4) {
                printf("Not enough keypoints to find homography! Ignoring this image\n");
                offset++; // TODO: this doesn't work since main thread will already have used the next value
                throw "";
            }
            t.logElapsed(id, "compute features");
            
            t.reset();
            do {
                std::cout << "Thread " << id << ": Locking" << std::endl;
                pthread_mutex_lock( &processedImageQueue.mutex );
                std::cout << "Thread " << id << ": Locked" << std::endl;
                if ((i == 0 && processedImageQueue.count == 0) // Edge case for first image
                    || (processedImageQueue.readPtr == i - 1) // If we're writing right after the last element, can enqueue our sequentially ordered image
                    ) {
                    std::cout << "Thread " << id << ": enqueue" << std::endl;
                    processedImageQueue.enqueueNoLock(mat,
                                            unique_keypoints_ptr(keypoints),
                                            unique_keypoints_ptr(),
                                            unique_keypoints_ptr(),
                                            unique_keypoints_ptr(),
                                            cv::Mat(),
                                            p,
                                            i);
                }
                else {
                    auto ms = 10;
                    std::cout << "Thread " << id << ": Sleeping " << ms << " milliseconds" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                    continue;
                }
                std::cout << "Thread " << id << ": Unlocking" << std::endl;
                pthread_mutex_unlock( &processedImageQueue.mutex );
                std::cout << "Thread " << id << ": Unlocked" << std::endl;
                break;
            } while (true);
            t.logElapsed(id, "enqueue procesed image");

            // cleanup
            free(k);
        }, /*extra args:*/ i - offset, mat, prevImage);
        
        // Save this image for next iteration
        prevImage = mat;
        
        if (firstImage.empty()) { // This is the first iteration.
            // Save firstImage once
            firstImage = mat;
        }
    }
    
    tp.isStop = true;
    int ret1;
    pthread_join(matcherThread, (void**)&ret1);
    
    tp.stop();
    
    // Print the final homography
    cv::Mat& M = lastImageToFirstImageTransformation;
    cv::Ptr<cv::Formatter> fmt = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
    fmt->set64fPrecision(4);
    fmt->set32fPrecision(4);
    auto str = fmt->format(M);
    std::cout << str << std::endl;
    
    // Save final homography to an image
    cv::Mat canvas;
    cv::warpPerspective(firstImage, canvas /* <-- destination */, M, firstImage.size());
    auto name = openFileWithUniqueName("scaled", ".png");
    std::cout << "Saving to " << name << std::endl;
    cv::imwrite(name, canvas);
    return 0;
}
#else
template <typename DataSourceT, typename DataOutputT>
int mainInteractive(DataSourceT* src,
                    SIFTState& s,
                    SIFTParams& p,
                    size_t skip,
                    DataOutputT& o
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
		struct sift_keypoint_std *k;
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
        o2.writer.release(); // Save the file
        #endif
    }
    
    return 0;
}
#endif // USE_COMMAND_LINE_ARGS
