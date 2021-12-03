//
//  siftMain.cpp
//  (Originally example2.cpp)
//  SIFT
//
//  Created by VADL on 9/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//
// This file runs on the SIFT RPis. It can also run on your computer. You can optionally show a preview window for fine-grain control.

#include "siftMain.hpp"

#include "common.hpp"

#include "KeypointsAndMatching.hpp"
#include "compareKeypoints.hpp"
#include "DataSource.hpp"
#include "DataOutput.hpp"
#include "utils.hpp"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <fstream>

// For stack traces on segfault, etc.
#include <backward.hpp> // https://github.com/bombela/backward-cpp
namespace backward {
backward::SignalHandling* sh;
} // namespace backward

//#include "./optick/src/optick.h"

#include "Queue.hpp"
thread_local SIFT_T sift;
Queue<ProcessedImage<SIFT_T>, 1024 /*256*/ /*32*/> processedImageQueue;
cv::Mat lastImageToFirstImageTransformation; // "Message box" for the accumulated transformations so far
#ifdef USE_COMMAND_LINE_ARGS
Queue<ProcessedImage<SIFT_T>, 16> canvasesReadyQueue;
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
#ifndef USE_COMMAND_LINE_ARGS
// Data source
//using DataSourceT = FolderDataSource;
//using DataSourceT = CameraDataSource;
using DataSourceT = VideoFileDataSource;
#else
// Fixed, not configurable:
using DataSourceT = CameraDataSource;
#endif

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
                #ifdef USE_COMMAND_LINE_ARGS
                , CommandLineConfig& cfg
                #endif
                );

#ifdef USE_COMMAND_LINE_ARGS
    CommandLineConfig cfg;
#endif
int main(int argc, char **argv)
{
    //namespace backward {
    backward::SignalHandling sh;
    backward::sh = &sh;
    //} // namespace backward
    
//#define SEGFAULT_TEST
//#define SEGFAULT_TEST2
#ifdef SEGFAULT_TEST
    char* a = nullptr;
    *a = 0;
#endif
    
#ifdef SLEEP_BEFORE_RUNNING
	std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BEFORE_RUNNING));
#endif
    
//    OPTICK_APP("SIFT");
    
    SIFTState s;
    SIFTParams p;
    
	// Set the default "skip"
    size_t skip = 0;//120;//60;//100;//38;//0;
    DataOutputT o;
    
    // Command-line args //
#ifdef USE_COMMAND_LINE_ARGS
    system("export DISPLAY=:0.0"); // Needed to show windows with GTK/X11 correctly
    
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
        else if (strcmp(argv[i], "--image-file-output") == 0) { // Outputs to video, usually also instead of preview window
            cfg.imageFileOutput = true;
        }
        else if (strcmp(argv[i], "--no-preview-window") == 0) { // Explicitly disable preview window
            cfg.noPreviewWindow = true;
        }
        else if (strcmp(argv[i], "--main-mission") == 0) {
            cfg.mainMission = true;
        }
//        else if (i+1 < argc && strcmp(argv[i], "--sleep-before-running") == 0) {
//            long long time = std::stoll(argv[i+1]); // Time in milliseconds
//            // Special value: -1 means sleep default time
//            if (time == -1) {
//                time = 30 * 1000;
//            }
//            std::this_thread::sleep_for(std::chrono::milliseconds(time));
//        }
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
    
    if (!cfg.mainMission) {
        p.params = sift_assign_default_parameters();
        mainInteractive(src.get(), s, p, skip, o, o2, cfg);
    }
    else {
        mainMission(src.get(), p, o2, cfg);
    }
#else
    DataSourceT src_ = makeDataSource<DataSourceT>(argc, argv, skip); // Read folder determined by command-line arguments
    DataSourceT* src = &src_;
    
    FileDataOutput o2("dataOutput/live", 1.0 /* fps */ /*, sizeFrame */);
    mainMission(src, p, o2);
#endif
    // //
    
	return 0;
}

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

#ifdef USE_COMMAND_LINE_ARGS
DataSourceBase* g_src;
#endif
void* matcherThreadFunc(void* arg) {
    do {
//        OPTICK_THREAD("Worker");
        
        ProcessedImage<SIFT_T> img1, img2;
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
        img2.applyDefaultMatchingParams();

        // Matching and homography
        t.reset();
        sift.findHomography(img1, img2
#ifdef USE_COMMAND_LINE_ARGS
                            , g_src, cfg
#endif
                            );
#ifdef USE_COMMAND_LINE_ARGS
        canvasesReadyQueue.enqueue(img2);
#endif
        // Accumulate homography
        if (lastImageToFirstImageTransformation.empty()) {
            lastImageToFirstImageTransformation = img2.transformation.inv();
        }
        else {
            lastImageToFirstImageTransformation *= img2.transformation.inv();
        }
        t.logElapsed("find homography");
        
        end:
        // Free memory and mark this as an unused ProcessedImage:
        img2.resetMatching();
    } while (!stoppedMain());
    return (void*)0;
}

cv::Rect g_desktopSize;

std::atomic<bool> g_stop;
void stopMain() {
    g_stop = true;
}
bool stoppedMain() {
    return g_stop;
}

#define tpPush(x, ...) tp.push(x, __VA_ARGS__)
//#define tpPush(x, ...) x(-1, __VA_ARGS__) // Single-threaded hack to get exceptions to show! Somehow std::future can report exceptions but something needs to be done and I don't know what; see https://www.ibm.com/docs/en/i/7.4?topic=ssw_ibm_i_74/apis/concep30.htm and https://stackoverflow.com/questions/15189750/catching-exceptions-with-pthreads and `ctpl_stl.hpp`'s strange `auto push(F && f) ->std::future<decltype(f(0))>` function
//ctpl::thread_pool tp(4); // Number of threads in the pool
ctpl::thread_pool tp(8);
// ^^ Note: "the destructor waits for all the functions in the queue to be finished" (or call .stop())
// Prints a stacktrace
//void logTrace() {
//    using namespace backward;
//    StackTrace st; st.load_here();
//    Printer p;
//    p.print(st, stderr);
//}
void ctrlC(int s, siginfo_t *si, void *arg){
    printf("Caught signal %d. Threads are stopping...\n",s);
    stopMain();
    
    // Print stack trace
    backward::sh->handleSignal(s, si, arg);
}
FileDataOutput* g_o2 = nullptr;
void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    printf("Caught %s at address %p\n", strsignal(signal), si->si_addr);
    
    if (g_o2) {
        g_o2->writer.release(); // Save the file
        std::cout << "Saved the video" << std::endl;
    }
    else {
        std::cout << "No video to save" << std::endl;
    }
    
    // Print stack trace
    backward::sh->handleSignal(signal, si, arg);
    
    exit(5);
}
template <typename DataSourceT, typename DataOutputT>
int mainMission(DataSourceT* src,
                SIFTParams& p,
                DataOutputT& o2
#ifdef USE_COMMAND_LINE_ARGS
, CommandLineConfig& cfg
#endif
) {
    #ifdef USE_COMMAND_LINE_ARGS
    g_src = src;
    #endif
    
    // Install ctrl-c handler
    // https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
    struct sigaction sigIntHandler;
    sigIntHandler.sa_sigaction = ctrlC;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    // Install segfault handler
    // https://stackoverflow.com/questions/2350489/how-to-catch-segmentation-fault-in-linux
    g_o2 = &o2;
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL); // Segfault handler! Tries to save stuff ASAP, unlike sigIntHandler.
    
    // Install SIGBUS (?), SIGILL (illegal instruction), and EXC_BAD_ACCESS (similar to segfault but is for "A crash due to a memory access issue occurs when an app uses memory in an unexpected way. Memory access problems have numerous causes, such as dereferencing a pointer to an invalid memory address, writing to read-only memory, or jumping to an instruction at an invalid address. These crashes are most often identified by the EXC_BAD_ACCESS (SIGSEGV) or EXC_BAD_ACCESS (SIGBUS) exceptions" ( https://developer.apple.com/documentation/xcode/investigating-memory-access-crashes )) handlers
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);

    if (CMD_CONFIG(showPreviewWindow())) {
        #ifdef USE_COMMAND_LINE_ARGS
        // Doesn't work //
        // Get desktop size
        // https://github.com/opencv/opencv/issues/10438
//        cv::namedWindow("", cv::WINDOW_NORMAL);
//        cv::setWindowProperty("", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
//        char arr[1] = {0};
//        imshow("", cv::Mat(1, 1, CV_8U, arr));
//        g_desktopSize = cv::getWindowImageRect("");
        // //
        
        int w, h;
        getScreenResolution(w, h);
        g_desktopSize = {0,0,w,h};
        
        std::cout << "Desktop size: " << g_desktopSize << std::endl;
        #endif
    }
    
    // SIFT
    cv::Mat firstImage;
    cv::Mat prevImage;
    pthread_t matcherThread;
    pthread_create(&matcherThread, NULL, matcherThreadFunc, NULL);
    
    auto start = std::chrono::steady_clock::now();
    auto last = std::chrono::steady_clock::now();
    auto fps = src->fps();
    const long long timeBetweenFrames_milliseconds = 1/fps * 1000;
    std::cout << "Target fps: " << fps << std::endl;
    //std::atomic<size_t> offset = 0; // Moves back the indices shown to SIFT threads
    for (size_t i = src->currentIndex; !stoppedMain(); i++) {
//        OPTICK_FRAME("MainThread"); // https://github.com/bombomby/optick
        
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
#ifdef STOP_AFTER
	if (since(start).count() > STOP_AFTER) {
        stopMain();
		break;
	}
#endif
        t.reset();
        cv::Mat mat = src->get(i);
        std::cout << "CAP_PROP_POS_MSEC: " << src->timeMilliseconds() << std::endl;
        t.logElapsed("get image");
        if (mat.empty()) {
            printf("No more images left to process. Exiting.\n");
            //stopMain();
            break;
        }
        o2.showCanvas("", mat);
        t.reset();
        cv::Mat greyscale = src->siftImageForMat(i);
        t.logElapsed("siftImageForMat");
        //auto path = src->nameForIndex(i);
        
        std::cout << "Pushing function to thread pool, currently has " << tp.n_idle() << " idle thread(s) and " << tp.q.size() << " function(s) queued" << std::endl;
        tpPush([&pOrig=p](int id, /*extra args:*/ size_t i, cv::Mat greyscale) {
//            OPTICK_EVENT();
            std::cout << "hello from " << id << std::endl;

            SIFTParams p(pOrig); // New version of the params we can modify (separately from the other threads)
            p.params = sift_assign_default_parameters();
            //v2Params(p.params);

            // compute sift keypoints
            std::cout << id << " findKeypoints" << std::endl;
#ifdef SIFTAnatomy_
            auto pair = sift.findKeypoints(id, p, greyscale);
            int n = pair.second.second; // Number of keypoints
            struct sift_keypoints* keypoints = pair.first;
            struct sift_keypoint_std *k = pair.second.first;
#elif defined(SIFTOpenCV_)
            auto vecPair = sift.findKeypoints(id, p, greyscale);
#endif
            std::cout << id << " findKeypoints end" << std::endl;
            
            t.reset();
            bool isFirstSleep = true;
            do {
                if (isFirstSleep) {
                    std::cout << "Thread " << id << ": Locking" << std::endl;
                }
                pthread_mutex_lock( &processedImageQueue.mutex );
                if (isFirstSleep) {
                    std::cout << "Thread " << id << ": Locked" << std::endl;
                }
                if ((i == 0 && processedImageQueue.count == 0) // Edge case for first image
                    || (processedImageQueue.readPtr == i - 1) // If we're writing right after the last element, can enqueue our sequentially ordered image
                    ) {
                    std::cout << "Thread " << id << ": enqueue" << std::endl;
                    #ifdef SIFTAnatomy_
                    processedImageQueue.enqueueNoLock(greyscale,
                                            shared_keypoints_ptr(keypoints),
                                            std::shared_ptr<struct sift_keypoint_std>(k),
                                            n,
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            cv::Mat(),
                                            p,
                                            i);
                    #elif defined(SIFTOpenCV_)
                    processedImageQueue.enqueueNoLock(greyscale,
                                                      vecPair.first,
                                                      vecPair.second,
                                                      std::vector< cv::DMatch >(),
                                            cv::Mat());
                    #endif
                }
                else {
                    auto ms = 10;
                    if (isFirstSleep) {
                        std::cout << "Thread " << id << ": Unlocking" << std::endl;
                    }
                    pthread_mutex_unlock( &processedImageQueue.mutex );
                    if (isFirstSleep) {
                        std::cout << "Thread " << id << ": Unlocked" << std::endl;
                        std::cout << "Thread " << id << ": Sleeping " << ms << " milliseconds at least once (and possibly locking and unlocking a few times more)..." << std::endl;
                        isFirstSleep = false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                    continue;
                }
                std::cout << "Thread " << id << ": Unlocking 2" << std::endl;
                pthread_mutex_unlock( &processedImageQueue.mutex );
                std::cout << "Thread " << id << ": Unlocked 2" << std::endl;
                break;
            } while (!stoppedMain());
            t.logElapsed(id, "enqueue processed image");

            end:
            ;
            // cleanup: nothing to do
        }, /*extra args:*/ i /*- offset*/, greyscale);
        
        // Save this image for next iteration
        prevImage = mat;
        
        #ifdef SEGFAULT_TEST2
        char* a = nullptr;
        *a = 0;
        #endif
        
        if (firstImage.empty()) { // This is the first iteration.
            // Save firstImage once
            firstImage = mat;
        }
        
#ifdef USE_COMMAND_LINE_ARGS
        // Show images
        if (!canvasesReadyQueue.empty()) {
            ProcessedImage<SIFT_T> img;
            canvasesReadyQueue.dequeue(&img);
            imshow("", img.canvas);
            //cv::waitKey(30);
            auto size = canvasesReadyQueue.size();
            std::cout << "Showing image from canvasesReadyQueue with " << size << " images left" << std::endl;
            char c = cv::waitKey(1 + 150.0 / (1 + size)); // Sleep less as more come in
            if (c == 'q') {
                // Quit
                std::cout << "Exiting (q pressed)" << std::endl;
                stopMain();
            }
        }
#endif
    }
    
    o2.writer.release(); // Save the file
    
    // Show the rest of the images only if we stopped because we finished reading all frames
    if (!stoppedMain()) {
        #ifdef USE_COMMAND_LINE_ARGS
        std::cout << "Showing the rest of the images" << std::endl;
        // Show images
        auto waitKey = [](int t = 30){
            char c = cv::waitKey(t);
            if (c == 'q') {
                // Quit
                std::cout << "Exiting (q pressed)" << std::endl;
                stopMain();
            }
        };
        while (//!canvasesReadyQueue.empty() // More images to show
               //||
               !tp.q.empty() // More functions to run
               && !stoppedMain()
               ) {
            ProcessedImage<SIFT_T> img;
            // Wait for dequeue
            bool stopped = false;
            while (canvasesReadyQueue.empty() && !(stopped = stoppedMain())) {
                std::this_thread::sleep_for(std::chrono::milliseconds(99));
                waitKey(1); // Check if user wants to quit
            }
            if (stopped) {
                break;
            }
            canvasesReadyQueue.dequeue(&img);
            imshow("", img.canvas);
            auto size = canvasesReadyQueue.size();
            std::cout << "Showing image from canvasesReadyQueue with " << size << " images left" << std::endl;
            waitKey(); // Check if user wants to quit
        }
        #else
        // Wait for no more in queue
        std::cout << "Main thread: waiting for no more in queue" << std::endl;
        while (!tp.q.empty() // More functions to run
               && !stoppedMain()
               ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "Main thread: done waiting for no more in queue" << std::endl;
        #endif
    }
    
    tp.stop(!stoppedMain() /*true to wait for queued functions as well*/); // Join thread pool's threads

    // Sometimes, the matcher thread is waiting on the condition variable still, and it will be waiting forever unless we interrupt it somehow (or broadcast on the condition variable..).
    pthread_cancel(matcherThread); // https://man7.org/linux/man-pages/man3/pthread_cancel.3.html
    
    int ret1;
    pthread_join(matcherThread, (void**)&ret1);
    
    // Print the final homography
    cv::Mat& M = lastImageToFirstImageTransformation;
    cv::Ptr<cv::Formatter> fmt = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
    fmt->set64fPrecision(4);
    fmt->set32fPrecision(4);
    auto str = fmt->format(M);
    std::cout << str << std::endl;
    
    // Save final homography matrix
    auto name = M.empty() ? openFileWithUniqueName("dataOutput/firstImage", ".png") : openFileWithUniqueName("dataOutput/scaled", ".png");
    auto matName = openFileWithUniqueName(name + ".matrix", ".txt");
    std::ofstream(matName.c_str()) << str << std::endl;
    
    // Save final homography to an image
    if (firstImage.empty()) {
        std::cout << "No first image" << std::endl;
        return 0;
    }
    if (M.empty()) {
        std::cout << "No final homography" << std::endl;
        
        // Save just the image
        std::cout << "Saving to " << name << std::endl;
        cv::imwrite(name, firstImage);
        
        return 0;
    }
    cv::Mat canvas;
    cv::warpPerspective(firstImage, canvas /* <-- destination */, M, firstImage.size());
//    cv::warpAffine(firstImage, canvas /* <-- destination */, M, firstImage.size());
    std::cout << "Saving to " << name << std::endl;
    cv::imwrite(name, canvas);
    return 0;
}

#ifdef USE_COMMAND_LINE_ARGS
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
        o2.writer.release(); // Save the file
        #endif
    }
    
    return 0;
}
#endif // USE_COMMAND_LINE_ARGS
