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
#include "main/siftMainCmdConfig.hpp"
#include "main/siftMainInteractive.hpp"
#include "main/siftMainSIFT.hpp"
#include "main/siftMainSignals.hpp"
#include "main/siftMainMatcher.hpp"
#include "main/siftMainCmdLineParser.hpp"

#include "KeypointsAndMatching.hpp"
#include "DataSource.hpp"
#include "DataOutput.hpp"
#include "utils.hpp"

#include <fstream>
#include <python3.7m/Python.h>
#include "../common.hpp"
#include "../subscale_driver/pyMainThreadInterface.hpp"
#include "../subscale_driver/py.h"
#include "main/SubscaleDriverInterface.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h> // for py::array_t
namespace py = pybind11;

// For stack traces on segfault, etc.
//#include <backward.hpp> // https://github.com/bombela/backward-cpp
namespace backward {
//backward::SignalHandling* sh;
} // namespace backward

//#include "./optick/src/optick.h"
#include "timers.hpp"

#include "IMUData.hpp"

#include "Queue.hpp"

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
using DataSourceT = CameraDataSource;
//using DataSourceT = VideoFileDataSource;

//using DataOutputT = PreviewWindowDataOutput;
using DataOutputT = FileDataOutput;
#else
// Fixed, not configurable:
// The effective source changes only if not using certain command-line arguments.
using DataSourceT = CameraDataSource;

// Fixed, not configurable:
// The effective output changes only if not using certain command-line arguments.
using DataOutputT = PreviewWindowDataOutput;
#endif
// //

#include "tools/malloc_with_free_all.h"
// https://gist.github.com/dgoguerra/7194777
static const char *humanSize(uint64_t bytes)
{
    char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
    char length = sizeof(suffix) / sizeof(suffix[0]);

    int i = 0;
    double dblBytes = bytes;

    if (bytes > 1024) {
        for (i = 0; (bytes / 1024) > 0 && i<length-1; i++, bytes /= 1024)
            dblBytes = bytes / 1024.0;
    }

    static char output[200];
    sprintf(output, "%.02lf %s", dblBytes, suffix[i]);
    return output;
}
// Call this on the end of a "frame" of processing in any thread.
template <typename ThreadInfoT /* can be threadID or thread name */>
void showTimers(ThreadInfoT threadInfo) {
    // Show timers for this thread
    Timer::logNanos(threadInfo, "*malloc total*", mallocTimerAccumulator);
    Timer::logNanos(threadInfo, "*free total*", freeTimerAccumulator);
    mallocTimerAccumulator = freeTimerAccumulator = 0; // Reset
    // Show malloc stats for this thread
    out_guard();
    std::cout << "Thread " << threadInfo << ": " << "numMallocsThisFrame" << " was " << numMallocsThisFrame << std::endl;
    std::cout << "Thread " << threadInfo << ": " << "numPointerIncMallocsThisFrame" << " was " << numPointerIncMallocsThisFrame << std::endl;
    std::cout << "Thread " << threadInfo << ": " << "used " << humanSize(mallocWithFreeAll_current - mallocWithFreeAll_origPtr) << " out of " << humanSize(mallocWithFreeAll_max - mallocWithFreeAll_origPtr) << std::endl;
    std::cout << "Thread " << threadInfo << ": " << "hit malloc pointer inc limit" << " " << mallocWithFreeAll_hitLimitCount << " times" << std::endl;
    // Reset them
    numMallocsThisFrame = 0;
    numPointerIncMallocsThisFrame = 0;
    mallocWithFreeAll_hitLimitCount = 0;
}
template <typename DataSourceT, typename DataOutputT>
int mainMission(DataSourceT* src,
                SIFTState& s,
                SIFTParams& p,
                DataOutputT& o
                #ifdef USE_COMMAND_LINE_ARGS
                , CommandLineConfig& cfg
                #endif
                );

int main(int argc, char **argv)
{
    //namespace backward {
    //backward::SignalHandling sh;
    //backward::sh = &sh;
    //} // namespace backward
    
//#define SEGFAULT_TEST
//#define SEGFAULT_TEST2
//#define PYTHON_TEST
#ifdef SEGFAULT_TEST
    char* a = nullptr;
    *a = 0;
#endif
#ifdef SIFTGPU_TEST
    return SIFTGPU::test(argc, argv);
#endif
#ifdef PYTHON_TEST
    common::connect_Python();
    S_RunFile("src/python/Precession.py", 0, nullptr);
    S_RunString("print(judge_image([1,2,3], [1.1,2,3]))");
    return 0;
#endif
    
    // Warm up (in case mkdir fails)
    getDataOutputFolder();
    
    // Main area //
    
#ifdef SLEEP_BEFORE_RUNNING
	std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BEFORE_RUNNING));
#endif
    
//    OPTICK_APP("SIFT");

    SIFTState s;
    SIFTParams p;
#ifdef SIFTAnatomy_
    p.params = sift_assign_default_parameters();
#endif
    
	// Set the default "skip"
    size_t skip = 0;//120;//60;//100;//38;//0;
    DataOutputT o;
    
    // Command-line args //
#ifdef USE_COMMAND_LINE_ARGS
    if (putenv("DISPLAY=:0.0") != 0) { // Equivalent to running `export DISPLAY=:0.0` in bash.  // Needed to show windows with GTK/X11 correctly
        perror("Failed to set X11 display");
        exit(EXIT_FAILURE);
    }
    
    std::unique_ptr<DataSourceBase> src;

    // Parse arguments and set `cfg` variables
    int ret = parseCommandLineArgs(argc, argv, s, p, skip, src);
    if (ret != 0) {
        return ret;
    }
    
    if (cfg.cameraTestOnly) {
        // Simple camera test to isolate issues
        // https://answers.opencv.org/question/1/how-can-i-get-frames-from-my-webcam/
        
        cv::VideoCapture cap;
        // open the default camera, use something different from 0 otherwise;
        // Check VideoCapture documentation.
        if(!cap.open(0)) {
            std::cout << "Failed to open camera. Exiting." << std::endl;
            return 1;
        }
        for(;;)
        {
              cv::Mat frame;
              cap >> frame;
              if( frame.empty() ) break; // end of video stream
              imshow("this is you, smile! :)", frame);
              if( cv::waitKey(10) == 27 ) break; // stop capturing by pressing ESC
        }
        // the camera will be closed automatically upon exit
        // cap.close();
        return 0;
    }
    
    if (cfg.cameraDataSource()) {
        src = std::make_unique<DataSourceT>(argc, argv); // Sets fps, etc. from command line args
    }
    
    FileDataOutput o2(getDataOutputFolder() + "/live", src->fps() /* fps */ /*, sizeFrame */);
    
    if (!cfg.mainMission) {
        #ifdef SIFTAnatomy_
        p.params = sift_assign_default_parameters();
        mainInteractive(src.get(), s, p, skip, o, o2, cfg);
        #else
        puts("SIFTAnatomy_ was not defined and mainInteractive requires this to be available for use. To fix this, run with --main-mission or recompile with SIFTAnatomy_ defined.");
        puts("Exiting.");
        return 1;
        #endif
    }
    else {
        static_assert(std::is_same<decltype(o2), FileDataOutput>::value, "Always use the FileDataOutput instead of PreviewWindowDataOutput here for the mainMission() call, since mainMission() has custom imshow code enabled if `CMD_CONFIG(showPreviewWindow())` instead of using PreviewWindowDataOutput.");
        mainMission(src.get(), s, p, o2, cfg);
    }
#else
    DataSourceT src_ = makeDataSource<DataSourceT>(argc, argv, skip); // Read folder determined by command-line arguments
    DataSourceT* src = &src_;
    
    FileDataOutput o2(getDataOutputFolder() + "/live", src->fps() /* fps */ /*, sizeFrame */);
    mainMission(src, s, p, o2);
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

cv::Rect g_desktopSize;

// This is a wrapper for calling Python using pybind11. Use it when you don't want unhandled Python exceptions to stop the entirety of SIFT (basically always use it).
// https://pybind11.readthedocs.io/en/stable/advanced/exceptions.html : "Any noexcept function should have a try-catch block that traps class:error_already_set (or any other exception that can occur). Note that pybind11 wrappers around Python exceptions such as pybind11::value_error are not Python exceptions; they are C++ exceptions that pybind11 catches and converts to Python exceptions. Noexcept functions cannot propagate these exceptions either. A useful approach is to convert them to Python exceptions and then discard_as_unraisable as shown below."
template <typename F>
void nonthrowing_python(F f) noexcept(true) {
    try {
        f();
    } catch (py::error_already_set &eas) {
        // Discard the Python error using Python APIs, using the C++ magic
        // variable __func__. Python already knows the type and value and of the
        // exception object.
        eas.discard_as_unraisable(__func__);
    } catch (const std::exception &e) {
        // Log and discard C++ exceptions.
        { out_guard();
            std::cout << "C++ exception from python: " << e.what() << std::endl; }
    }
}

#define tpPush(x, ...) tp.push(x, __VA_ARGS__)
//#define tpPush(x, ...) x(-1, __VA_ARGS__) // Single-threaded hack to get exceptions to show! Somehow std::future can report exceptions but something needs to be done and I don't know what; see https://www.ibm.com/docs/en/i/7.4?topic=ssw_ibm_i_74/apis/concep30.htm and https://stackoverflow.com/questions/15189750/catching-exceptions-with-pthreads and `ctpl_stl.hpp`'s strange `auto push(F && f) ->std::future<decltype(f(0))>` function
//ctpl::thread_pool tp(4); // Number of threads in the pool
//ctpl::thread_pool tp(8);
ctpl::thread_pool tp(6);
// ^^ Note: "the destructor waits for all the functions in the queue to be finished" (or call .stop())
template< class... Args >
void processedImageQueue_enqueueIndirect(size_t i, Args&&... args) {
    // Check if we can enqueue now
    { out_guard(); std::cout << "processedImageQueue_enqueueIndirect called: i=" << i << ": locking processedImageQueue" << std::endl; }
    pthread_mutex_lock( &processedImageQueue.mutex );
    if ((i == 0 && processedImageQueue.count == 0) // Edge case for first image
        || (processedImageQueue.writePtr == (i) % processedImageQueueBufferSize) // If we're writing right after the last element, can enqueue our sequentially ordered image
        ) {
        processedImageQueue.enqueueNoLock(args...);
        pthread_mutex_unlock( &processedImageQueue.mutex );
        { out_guard(); std::cout << "processedImageQueue_enqueueIndirect fast path: i=" << i << ": unlocked processedImageQueue" << std::endl; }
    }
    else {
        pthread_mutex_unlock( &processedImageQueue.mutex );
        { out_guard(); std::cout << "processedImageQueue_enqueueIndirect: need to wait for enqueue, i=" << i << ": unlocked processedImageQueue" << std::endl; }
        
        // https://stackoverflow.com/questions/47496358/c-lambdas-how-to-capture-variadic-parameter-pack-from-the-upper-scope
        // NOTE: this is probably very inefficient and also allocates on the heap:
        tpPush([args = std::make_tuple(std::forward<Args>(args) ...)](int id, size_t i)mutable {
            return std::apply([id, i](auto&& ... args){
                while (true) {
                    { out_guard(); std::cout << "processedImageQueue_enqueueIndirect: i=" << i << ", id=" << id << ": locking processedImageQueue" << std::endl; }
                    pthread_mutex_lock( &processedImageQueue.mutex );
                    { out_guard(); std::cout << "processedImageQueue_enqueueIndirect: locked, i=" << i << ", id=" << id << ", readPtr=" << processedImageQueue.readPtr << ", writePtr=" << processedImageQueue.writePtr << ", count=" << processedImageQueue.count << " : locked processedImageQueue" << std::endl; }
                    if ((i == 0 && processedImageQueue.count == 0) // Edge case for first image
                        || (processedImageQueue.writePtr == (i) % processedImageQueueBufferSize) // If we're writing right after the last element, can enqueue our sequentially ordered image
                    ) {
                        { out_guard(); std::cout << "processedImageQueue_enqueueIndirect: i=" << i << ", id=" << id << ": enqueueNoLock" << std::endl; }
                        processedImageQueue.enqueueNoLock(args...);
                        pthread_mutex_unlock( &processedImageQueue.mutex );
                        { out_guard(); std::cout << "processedImageQueue_enqueueIndirect: i=" << i << ", id=" << id << ": unlocked processedImageQueue 1" << std::endl; }
                        break;
                    }
                    pthread_mutex_unlock( &processedImageQueue.mutex );
                    { out_guard(); std::cout << "processedImageQueue_enqueueIndirect: i=" << i << ", id=" << id << ": unlocked processedImageQueue 2" << std::endl; }
                    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Try again next time
                }
            }, std::move(args));
        }, i);
    }
}
#ifdef USE_PTR_INC_MALLOC
thread_local void* bigMallocBlock = nullptr; // Never freed on purpose
#endif
template <typename DataSourceT, typename DataOutputT>
int mainMission(DataSourceT* src,
                SIFTState& s,
                SIFTParams& p,
                DataOutputT& o2
#ifdef USE_COMMAND_LINE_ARGS
, CommandLineConfig& cfg
#endif
) {
    #ifdef USE_COMMAND_LINE_ARGS
    g_src = src;
    #endif
    
    g_o2 = &o2;
//    if (atexit(atexit_handler) != 0) { // https://man7.org/linux/man-pages/man3/atexit.3.html : "The atexit() function registers the given function to be called
//        // at normal process termination, either via exit(3) or via return
//        // from the program's main()."
//        printf("Failed to set atexit handler");
//        exit(EXIT_FAILURE);
//    }
    installSignalHandlers();

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
    
    common::connect_Python();
    // Warm up python (seems to take ~1 second)
    t.reset();
    S_RunFile("src/python/Precession.py", 0, nullptr);
    t.logElapsed("Precession.py warmup");
    
    // SIFT
    cv::Mat firstImage;
    cv::Mat prevImage;
    pthread_t matcherThread;
    pthread_create(&matcherThread, NULL, matcherThreadFunc, &s);
    #ifdef __linux__
    pthread_setname_np(matcherThread, "matcherThread"); // https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
    #endif
    
    pthread_t subscaleDriverInterfaceThread;
    if (driverInput_fd != -1) {
        static_assert(sizeof(void*) >= sizeof(int));
        pthread_create(&subscaleDriverInterfaceThread, NULL, subscaleDriverInterfaceThreadFunc, /*warning is wrong, this is ok*/(void*)driverInput_fd);
        #ifdef __linux__
        pthread_setname_np(subscaleDriverInterfaceThread, "subscaleDriverInterfaceThread"); // https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
        #endif
    }
    
    auto start = std::chrono::steady_clock::now();
    auto last = std::chrono::steady_clock::now();
    auto timeSinceLastFlush = std::chrono::steady_clock::now();
    auto fps = src->fps();
    const long long timeBetweenFrames_milliseconds = 1/fps * 1000;
    { out_guard();
        std::cout << "Target fps: " << fps << std::endl; }
    //std::atomic<size_t> offset = 0; // Moves back the indices shown to SIFT threads
    for (size_t i = src->currentIndex; !stoppedMain(); i++) {
//        OPTICK_FRAME("MainThread"); // https://github.com/bombomby/optick
        
        { out_guard();
            std::cout << "i: " << i << std::endl; }
        #ifdef USE_COMMAND_LINE_ARGS
        for (auto& pair : cfg.skipImageIndices) {
            if (pair.first == i) {
                { out_guard();
                    std::cout << "Skipping to index: " << pair.second << std::endl; }
                for (size_t i2 = pair.first; i2 < pair.second; i2++) {
                    cv::Mat mat = src->get(i2); // Consume images
                    // Enqueue null image indirectly
                    processedImageQueue_enqueueIndirect(i2, mat,
                                            shared_keypoints_ptr(),
                                            std::shared_ptr<struct sift_keypoint_std>(),
                                            0,
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            cv::Mat(),
                                            p,
                                            i2,
                                            std::shared_ptr<IMUData>());
                    
                    // Show a bit (twice because with only one call to showAnImageUsingCanvasesReadyQueue() we may fill up the canvasesReadyQueue since it only dequeues once per call to showAnImageUsingCanvasesReadyQueue())
                    showAnImageUsingCanvasesReadyQueue(src, o2);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    showAnImageUsingCanvasesReadyQueue(src, o2);
                }
                i = pair.second; // -1 due to the i++
                continue;
            }
        }
        #endif
        auto sinceLast_milliseconds = since(last).count();
        if (src->wantsCustomFPS()) {
            if (sinceLast_milliseconds < timeBetweenFrames_milliseconds) {
                // Sleep
                auto sleepTime = timeBetweenFrames_milliseconds - sinceLast_milliseconds;
                { out_guard();
                    std::cout << "Sleeping for " << sleepTime << " milliseconds" << std::endl; }
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }
        last = std::chrono::steady_clock::now();
        if (sinceLast_milliseconds > timeBetweenFrames_milliseconds) {
            out_guard();
            std::cout << "WARNING: Lagging behind in video capture by " << sinceLast_milliseconds - timeBetweenFrames_milliseconds << " milliseconds" << std::endl;
        }
#ifdef STOP_AFTER
	if (since(start).count() > STOP_AFTER) {
        stopMain();
		break;
	}
#endif
        t.reset();
        cv::Mat mat = src->get(i);
        { out_guard();
            std::cout << "CAP_PROP_POS_MSEC: " << src->timeMilliseconds() << std::endl; }
        t.logElapsed("get image");
        if (mat.empty()) {
            if (!CMD_CONFIG(finishRestOnOutOfImages)) {
                printf("No more images left to process. Exiting.\n");
                stopMain();
                break;
            }
            else {
                if ((tp.q.size() != 0 && tp.n_idle() != tp.size()) || processedImageQueue.size() > 0) {
                //if (tp.n_idle() != tp.size()) {
                    printf("No more images left to process, but SIFT threads remain. Waiting/showing images...\n");
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    i--; // "Prevent" i from incrementing on for loop update
                    goto skipImage; // Show images with canvas etc.
                }
                else {
                    printf("No more images left to process, and no SIFT threads remain. Exiting.\n");
                    stopMain();
                    break;
                }
            }
        }
        // Possibly saving it:
        if (!CMD_CONFIG(siftVideoOutput)) {
            // Save original frame to the video output file
            
            // If enabled: every n frames, flush the video (creating more videos in the process which have to be joined together outside this program)
            bool maybeFlush = CMD_CONFIG(shouldFlushVideoOutputEveryNSeconds());
            bool flush = maybeFlush;

            if (maybeFlush) {
                auto now = std::chrono::steady_clock::now();
                auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - timeSinceLastFlush);
                auto destMillis = cfg.flushVideoOutputEveryNSeconds*1000;
                if (millis.count() > destMillis || cfg.flushVideoOutputEveryNSeconds == 0 || CMD_CONFIG(saveFirstImage)) {
                    flush = true;
                    { out_guard();
                        std::cout << "Flushing with lag " << millis.count() - destMillis << " milliseconds" << std::endl; }
                    
                    // Flush the matrix too //
                    lastImageToFirstImageTransformationMutex.lock();
                    if (!lastImageToFirstImageTransformation.empty() || CMD_CONFIG(saveFirstImage)) {
                        // Print the intermediate homography matrix
                        cv::Mat& M = lastImageToFirstImageTransformation;
                        cv::Ptr<cv::Formatted> str;
                        if (CMD_CONFIG(saveFirstImage)) {
                            std::cout << "Flushing first image\n";
                        }
                        else {
                            matrixToString(M, str);
                            { out_guard();
                                std::cout << "Flushing homography: " << str << std::endl; }
                        }
                        
                        // Save the intermediate homography matrix
                        std::string name;
                        saveMatrixGivenStr(CMD_CONFIG(saveFirstImage) ? cv::Mat() : M, name /* <--output */, str);
#define SAVE_INTERMEDIATE_SCALED_IMAGES
#ifdef SAVE_INTERMEDIATE_SCALED_IMAGES
                        // Save image for debugging only
                        cv::Mat canvas;
                        bool crop;
                        crop = src->shouldCrop();
                        //crop = false; // Hardcode, since it is weird when you scale the cropped image?
                        cv::Mat& firstImage_ = CMD_CONFIG(saveFirstImage) ? mat : firstImage; // firstImage is empty on first frame grab since it is set *after* running SIFT on it
                        cv::Mat img = crop ? firstImage_(src->crop()) : firstImage_;
                        if (!CMD_CONFIG(saveFirstImage)) {
                            cv::warpPerspective(img, canvas /* <-- destination */, M, img.size());
                            //    cv::warpAffine(firstImage, canvas /* <-- destination */, M, firstImage.size());
                        }
                        else {
                            canvas = img;
                            cfg.saveFirstImage = false; // Did already, don't do it again
                        }
                        { out_guard();
                            std::cout << "Saving to " << name << std::endl; }
                        cv::imwrite(name, canvas);
#endif
                    }
                    else {
                        std::cout << "Homography not flushed since it is empty\n";
                    }
                    lastImageToFirstImageTransformationMutex.unlock();
                    // //
                    
                    // Mark this as a finished flush
                    timeSinceLastFlush = now;
                }
                else {
                    flush = false;
                }
            }
            
            // Save frame to video writer or etc. in o2:
            //cv::Rect rect = src->shouldCrop() ? src->crop() : cv::Rect();
            //o2.showCanvas("", mat, flush, rect.empty() ? nullptr : &rect);
            o2.showCanvas("", mat, flush, nullptr);
        }
        t.reset();
        if (!CMD_CONFIG(imageCaptureOnly)) {
            cv::Mat greyscale = src->siftImageForMat(i);
            t.logElapsed("siftImageForMat");
            //auto path = src->nameForIndex(i);
            
            // Apply filters to possibly discard the image //
            static bool discardImage;
            discardImage = false;
            // Blur detection
            t.reset();
            cv::Mat laplacianImage;
            // https://stackoverflow.com/questions/24080123/opencv-with-laplacian-formula-to-detect-image-is-blur-or-not-in-ios/44579247#44579247 , https://www.pyimagesearch.com/2015/09/07/blur-detection-with-opencv/ , https://github.com/WillBrennan/BlurDetection2/blob/master/blur_detection/detection.py
            auto type = CV_32F;
            assert(type == greyscale.type()); // The type must be the same as `greyscale` else you get an error like `OpenCV(4.5.2) /build/source/modules/imgproc/src/filter.simd.hpp:3173: error: (-213:The function/feature is not implemented) Unsupported combination of source format (=5), and destination format (=6) in function 'getLinearFilter'`
            cv::Laplacian(greyscale, laplacianImage, type);
            cv::Scalar mean, stddev; // 0:1st channel, 1:2nd channel and 2:3rd channel
            cv::meanStdDev(laplacianImage, mean, stddev, cv::Mat());
            double variance = stddev.val[0] * stddev.val[0];
            // As variance goes up, image is "less blurry"
            const double nearBlackImageVariance = 0.0007;
            double threshold = nearBlackImageVariance; //2900; // TODO: dynamically adjust threshold

            { out_guard(); // Due to multiple prints in one logical "output"
                printf("Image %zu: Blur detection: variance %f; ", i, variance);
                if (variance <= threshold) {
                    // Blurry
                    puts("blurry");
                    discardImage = true;
                } else {
                    // Not blurry
                    puts("not blurry");
                }
            }
            t.logElapsed("blur detection");
            
            // IMU
            std::shared_ptr<IMUData> imu_;
            if (driverInput_fd != -1) { //driverInput_file) {
                imu_ = std::make_shared<IMUData>();
                // NOTE: this blocks until the fd gets data.
                t.reset();
                { out_guard();
                    std::cout << "grabbing from subscale driver interface thread..." << std::endl; }
                subscaleDriverInterfaceMutex_imuData.lock();
                int count = subscaleDriverInterface_count;
                static bool firstGrab = true;
                static py::object my_or;
                if (count > 0) {
                    // We have something to use
                    IMUData& imu__ = subscaleDriverInterface_imu;
                    *imu_ = imu__;
                    IMUData& imu = *imu_.get();
                    subscaleDriverInterfaceMutex_imuData.unlock();
                    { out_guard();
                        std::cout << "Linear accel NED: " << imu.linearAccelNed << " (data rows read: " << count << ")" << std::endl; }
                    
                    // Use the IMU data:
                    nonthrowing_python([&imu](){
                        using namespace pybind11::literals; // to bring in the `_a` literal
                        
                        py::module_ np = py::module_::import("numpy");  // like 'import numpy as np'
                        py::module_ precession = py::module_::import("src.python.Precession");
                        
                        py::list l;
                        l.append(imu.yprNed.x); l.append(imu.yprNed.y); l.append(imu.yprNed.z);
                        py::array_t<float> arr = np.attr("array")(l, "dtype"_a="float32");
                        
                        if (firstGrab) {
                            // Init the first orientation
                            my_or = precession.attr("init_orientation")(arr);
                            firstGrab = false;
                        }
                        
                        py::list threshold_vec;
                        threshold_vec.append(60); threshold_vec.append(60); threshold_vec.append(60);
                        
                        // Judge image
                        py::tuple shouldAcceptAndOrientation = precession.attr("judge_image")(my_or, arr, threshold_vec);
                        py::bool_ shouldAccept = shouldAcceptAndOrientation[0];
                        my_or = shouldAcceptAndOrientation[1];
                        if (shouldAccept == true) {
                            std::cout << "judge_image: Python likes this image\n";
                        }
                        else {
                            std::cout << "judge_image: Python doesn't like this image\n";
                            discardImage = true;
                        }
                        
                        // compare_to_NED
                        // (Make sure it's facing down)
                        py::print("arr:", arr);
                        py::object ffypr = precession.attr("get_ffYPR_theta")(arr);
                        py::print("ffypr:", ffypr);
                        py::bool_ closeToFacingDown = precession.attr("compare_to_NED")(arr, 40, 60);
                        if (closeToFacingDown == true) {
                            std::cout << "compare_to_NED: Python likes this image\n";
                        }
                        else {
                            std::cout << "compare_to_NED: Python doesn't like this image\n";
                            discardImage = true;
                        }
                    });
                }
                t.logElapsed("grabbing from subscale driver interface thread and running Precession.judge_image");
            }
            // //
            
            if (discardImage) {
                // Enqueue null image indirectly
                processedImageQueue_enqueueIndirect(i, greyscale,
                                            shared_keypoints_ptr(),
                                            std::shared_ptr<struct sift_keypoint_std>(),
                                            0,
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            cv::Mat(),
                                            p,
                                            i,
                                            imu_);
                
                // Don't push a function for this image or save it as a possible firstImage
                goto skipImage;
            }

            // Prepare image by applying warp using IMU data
            //            if (imu_) {
            //                cv::Mat_ warp = cv::Mat_<float> A = (cv::Mat_<float>(3, 3) <<
            //                                cos(alpha)*cos(beta), cos(alpha)*sin(beta)*sin(gamma)-sin(alpha)*cos(gamma), cos(alpha)*sin(beta)*cos(gamma)+sin(alpha)*sin(gamma),
            //                sin(alpha)*cosine(beta), sin(alpha)*sin(beta)*sin(gamma)+cos(alpha)*cos(gamma), sin(alpha)*sin(beta)*cos(gamma)-cos(alpha)*sin(gamma),
            //                3, 9);
            //            }
            
            { out_guard();
                std::cout << "Pushing function to thread pool, currently has " << tp.n_idle() << " idle thread(s) and " << tp.q.size() << " function(s) queued" << std::endl; }
            tpPush([&pOrig=p](int id, /*extra args:*/ size_t i, cv::Mat greyscale, std::shared_ptr<IMUData> imu_) {
    //            OPTICK_EVENT();
                { out_guard();
                    std::cout << "hello from " << id << std::endl; }
                installSignalHandlers();

    #ifdef USE_PTR_INC_MALLOC
                bigMallocBlock = beginMallocWithFreeAll(8*1024*1024*32 /* About 268 MB */, bigMallocBlock); // 8 is 8 bytes to align to a reasonable block size malloc might be using
    #endif
                SIFTParams p(pOrig); // New version of the params we can modify (separately from the other threads)
                // Overrides for the params of `pOrig` (into `p`) go below:
    //            v2Params(p.params);
    //            lowEdgeParams(p.params);
    //            p.params->C_edge = 8;    // 5 <-- too many outliers when run in the lab
    //	    p.params->n_spo = 2;
    //	    p.params->delta_min = 0.3;

                // compute sift keypoints
                { out_guard();
                    std::cout << "Thread " << id << ": running findKeypoints" << std::endl; }
    #ifdef SIFTAnatomy_
                auto pair = sift.findKeypoints(id, p, greyscale);
                int n = pair.second.second; // Number of keypoints
                
                // SIFT threads can provide an upper bound on the number of matches produced by the matching phase by too low numbers of keypoints. 50 keypoints minimum, raises parameters over time beforehand.
                float DEC = 1;
                float INC = DEC;
                if (n > 500) {
                    printf("Too many keypoints\n");
                    if (p.params->C_edge <= 1) {
                        printf("C_edge too low, runtime will probably be slow\n");
                    }
                    else {
                        p.params->C_edge -= DEC;
                    }
                }
                if (n < 80) {
                    printf("Low keypoints\n");
                    p.params->C_edge += INC; // We noticed that as C_edge goes up, we get more keypoints.
                }
                if (n < 50) {
                    printf("Critically low keypoints\n");
                }
                if (n < 4) {
                    // Not enough keypoints, ignore it
                    printf("Not enough keypoints generated by SIFT. This image will be considered \"null\" by the matcher.\n");
                    // Option 1: Now we can retry at most once or so, then skip this image.
                    // Option 2: Just skip this image. We will do this, since the parameters will be adjusted already from the above if statements like C_edge.
                    // DONETODO: don't ignore it, but retry match with previous image? You can store the previous image as a cv::Mat ref inside the current one to allow for this if you want..
                    //[done]need to change i
                    // So we enqueue a placeholder and let the matcher skip it
                    // Nothing to do here, the rest works
                }
                struct sift_keypoints* keypoints = pair.first;
                struct sift_keypoint_std *k = pair.second.first;
    #elif defined(SIFTOpenCV_)
                auto vecPair = sift.findKeypoints(id, p, greyscale);
    #elif defined(SIFTGPU_)
                static thread_local SIFTState s;
                auto pair = sift.findKeypoints(id, s, p, greyscale);
                auto& keys1 = pair.first;
                auto& descriptors1 = pair.second.first;
                auto& num1 = pair.second.second;
    #else
    #error "No known SIFT implementation"
    #endif
                { out_guard();
                    std::cout << "Thread " << id << ": findKeypoints end" << std::endl; }
                
    #ifdef USE_PTR_INC_MALLOC
                endMallocWithFreeAll();
    #endif
                t.reset();
                bool isFirstSleep = true;
                do {
                    if (isFirstSleep) {
                        { out_guard();
                            std::cout << "Thread " << id << ": Locking" << std::endl; }
                    }
                    pthread_mutex_lock( &processedImageQueue.mutex );
                    if (isFirstSleep) {
                        { out_guard();
                            std::cout << "Thread " << id << ": Locked" << std::endl; }
                    }
                    { out_guard();
                        std::cout << "@@@@@@@@@@@@@@@@Thread " << id << ": readPtr " << processedImageQueue.readPtr << ", i " << i << ", writePtr " << processedImageQueue.writePtr << ", processedImageQueue.count " << processedImageQueue.count << std::endl;
                    }
                    if ((i == 0 && processedImageQueue.count == 0) // Edge case for first image
                        || (processedImageQueue.writePtr == (i) % processedImageQueueBufferSize) // If we're writing right after the last element, can enqueue our sequentially ordered image
                        ) {
                        { out_guard();
                            std::cout << "Thread " << id << ": enqueue" << std::endl; }
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
                                                i,
                                                imu_);
                        #elif defined(SIFTOpenCV_)
                        processedImageQueue.enqueueNoLock(greyscale,
                                                          vecPair.first,
                                                          vecPair.second,
                                                          std::vector< cv::DMatch >(),
                                                cv::Mat());
                        #elif defined(SIFTGPU_)
                        processedImageQueue.enqueueNoLock(greyscale,
                                                          keys1,
                                                          descriptors1,
                                                          num1,
                                                cv::Mat());
                        #else
                        #error "No known SIFT implementation"
                        #endif
                    }
                    else {
                        auto ms = 10;
                        if (isFirstSleep) {
                            { out_guard();
                                std::cout << "Thread " << id << ": Unlocking" << std::endl; }
                        }
                        pthread_mutex_unlock( &processedImageQueue.mutex );
                        if (isFirstSleep) {
                            out_guard();
                            std::cout << "Thread " << id << ": Unlocked\n";
                            std::cout << "Thread " << id << ": Sleeping " << ms << " milliseconds at least once (and possibly locking and unlocking a few times more)..." << std::endl;
                            isFirstSleep = false;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                        continue;
                    }
                    { out_guard();
                        std::cout << "Thread " << id << ": Unlocking 2" << std::endl; }
                    pthread_mutex_unlock( &processedImageQueue.mutex );
                    { out_guard();
                        std::cout << "Thread " << id << ": Unlocked 2" << std::endl; }
                    break;
                } while (!stoppedMain());
                t.logElapsed(id, "enqueue processed image");

                end:
                ;
                // cleanup: nothing to do
                // Log malloc, etc. timers and reset them
                showTimers(std::string("SIFT ") + std::to_string(id));
            }, /*extra args:*/ i /*- offset*/, greyscale, imu_);
        }
        
        // Save this image for next iteration
        prevImage = mat;
        
        #ifdef SEGFAULT_TEST2
        char* a = nullptr;
        *a = 0;
        #endif
        
        if (firstImage.empty() && !mat.empty()) { // This is the first iteration.
            // Save firstImage once
            firstImage = mat;
        }
        
        skipImage:
        // Show images if we have them and if we are showing a preview window
        showAnImageUsingCanvasesReadyQueue(src, o2);
        
        showTimers("main");
    }
    
    o2.release(); // Save the file
    
    // Show the rest of the images only if we stopped because we finished reading all frames and if we are showing a preview window
    if (!stoppedMain() && CMD_CONFIG(showPreviewWindow())) {
        #ifdef USE_COMMAND_LINE_ARGS
        std::cout << "Showing the rest of the images\n";
        // Show images
        auto waitKey = [](int t = 30){
            char c = cv::waitKey(t);
            if (c == 'q') {
                // Quit
                { out_guard();
                    std::cout << "Exiting (q pressed)" << std::endl; }
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
            cv::Mat realCanvas = prepareCanvas(img);
            commonUtils::imshow("", realCanvas);
            if (CMD_CONFIG(siftVideoOutput)) {
                // Save frame with SIFT keypoints rendered on it to the video output file
                cv::Rect rect = src->shouldCrop() ? src->crop() : cv::Rect();
                o2.showCanvas("", realCanvas, false, rect.empty() ? nullptr : &rect);
            }
            auto size = canvasesReadyQueue.size();
            { out_guard();
                std::cout << "Showing image from canvasesReadyQueue with " << size << " images left" << std::endl; }
            waitKey(); // Check if user wants to quit
        }
        #else
        // Wait for no more in queue
        { out_guard();
            std::cout << "Main thread: waiting for no more in queue" << std::endl; }
        while (!tp.q.empty() // More functions to run
               && !stoppedMain()
               ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        { out_guard();
            std::cout << "Main thread: done waiting for no more in queue" << std::endl; }
        #endif
    }

//    { out_guard();
//        std::cout << "Broadcasting stoppedMain()" << std::endl; }
    
    // Wake up matcher thread and other SIFT threads so they see that we stoppedMain()
//    pthread_cond_broadcast(&processedImageQueue.condition); // Note: "The signal and broadcast operations do not require a mutex. A condition variable also is not permanently associated with a specific mutex; the external mutex does not protect the condition variable." ( https://stackoverflow.com/questions/2763714/why-do-pthreads-condition-variable-functions-require-a-mutex#:~:text=The%20signal%20and%20broadcast%20operations,not%20protect%20the%20condition%20variable. ) + verified by POSIX spec: "The pthread_cond_broadcast() or pthread_cond_signal() functions may be called by a thread whether or not it currently owns the mutex that threads calling pthread_cond_wait() or pthread_cond_timedwait() have associated with the condition variable during their waits; however, if predictable scheduling behavior is required, then that mutex shall be locked by the thread calling pthread_cond_broadcast() or pthread_cond_signal()." ( https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_broadcast.html )

    { out_guard();
        std::cout << "Stopping thread pool" << std::endl; }
    
    tp.stop(CMD_CONFIG(finishRestAlways) || !stoppedMain() /*true to wait for queued functions as well*/); // Join thread pool's threads

    { out_guard();
        std::cout << "Cancelling matcher thread" << std::endl; }
    
    // Sometimes, the matcher thread is waiting on the condition variable still, and it will be waiting forever unless we interrupt it somehow (or broadcast on the condition variable..).
    pthread_cancel(matcherThread); // https://man7.org/linux/man-pages/man3/pthread_cancel.3.html
    if (driverInput_fd != -1) {
        { out_guard();
            std::cout << "Cancelling subscaleDriverInterfaceThread" << std::endl; }
        pthread_cancel(subscaleDriverInterfaceThread); // (by default, pthread_cancel() makes the thread exit only when it calls a pthread_-prefixed function (a "cancellation point") ( https://man7.org/linux/man-pages/man3/pthread_cancel.3.html ))
    }

    { out_guard();
        std::cout << "Joining matcher thread" << std::endl; }
    
    int ret1;
    pthread_join(matcherThread, (void**)&ret1);
    if (driverInput_fd != -1) {
        { out_guard();
            std::cout << "Joining subscaleDriverInterfaceThread" << std::endl; }
        int ret2;
        pthread_join(subscaleDriverInterfaceThread, (void**)&ret2);
    }
    
    // Print the final homography
    //[unnecessary since matcherThread is stopped now:] lastImageToFirstImageTransformationMutex.lock();
    cv::Mat& M = lastImageToFirstImageTransformation;
    cv::Ptr<cv::Formatted> str;
    matrixToString(M, str);
    { out_guard();
        std::cout << "Final homography: " << str << std::endl; }
    
    // Save final homography matrix
    std::string name;
    saveMatrixGivenStr(M, name /* <--output */, str);
    //lastImageToFirstImageTransformationMutex.unlock();
    
    // Save final homography to an image
    if (firstImage.empty()) {
        std::cout << "No first image\n";
        return 0;
    }
    if (M.empty()) {
        std::cout << "No final homography\n";
        
        // Save just the image
        { out_guard();
            std::cout << "Saving to " << name << std::endl; }
        cv::imwrite(name, firstImage);
        
        return 0;
    }
    cv::Mat canvas;
    bool crop;
    crop = src->shouldCrop();
    //crop = false; // Hardcode, since it is weird when you scale the cropped image?
    cv::Mat img = crop ? firstImage(src->crop()) : firstImage;
    cv::warpPerspective(img, canvas /* <-- destination */, M, img.size());
//    cv::warpAffine(firstImage, canvas /* <-- destination */, M, firstImage.size());
    { out_guard();
        std::cout << "Saving to " << name << std::endl; }
    cv::imwrite(name, canvas);
    return 0;
}
