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

#include "tools/printf.h"
#include "IMUData.hpp"

#include "Queue.hpp"
thread_local SIFT_T sift;
constexpr size_t processedImageQueueBufferSize = 1024 /*256*/ /*32*/;
Queue<ProcessedImage<SIFT_T>, processedImageQueueBufferSize> processedImageQueue;
cv::Mat lastImageToFirstImageTransformation; // "Message box" for the accumulated transformations so far
std::mutex lastImageToFirstImageTransformationMutex;
#ifdef USE_COMMAND_LINE_ARGS
Queue<ProcessedImage<SIFT_T>, 16> canvasesReadyQueue;
#endif
int driverInput_fd = -1; // File descriptor for reading from the driver program, if any (else it is -1)
int driverInput_fd_fcntl_flags = 0;
FILE* driverInput_file = NULL;

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

#ifdef SIFTAnatomy_
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
#endif

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
    std::cout << "Thread " << threadInfo << ": " << "numMallocsThisFrame" << " was " << numMallocsThisFrame << std::endl;
    std::cout << "Thread " << threadInfo << ": " << "numPointerIncMallocsThisFrame" << " was " << numPointerIncMallocsThisFrame << std::endl;
    std::cout << "Thread " << threadInfo << ": " << "used " << humanSize(mallocWithFreeAll_current - mallocWithFreeAll_origPtr) << " out of " << humanSize(mallocWithFreeAll_max - mallocWithFreeAll_origPtr) << std::endl;
    std::cout << "Thread " << threadInfo << ": " << "hit malloc pointer inc limit" << " " << mallocWithFreeAll_hitLimitCount << " times" << std::endl;
    // Reset them
    numMallocsThisFrame = 0;
    numPointerIncMallocsThisFrame = 0;
    mallocWithFreeAll_hitLimitCount = 0;
}
// https://www.cplusplus.com/reference/ctime/strftime/
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#include <sys/stat.h> // https://pubs.opengroup.org/onlinepubs/009695299/functions/mkdir.html
std::string cachedDataOutputFolderPath;
const std::string& getDataOutputFolder() {
    if (cachedDataOutputFolderPath.empty()) {
        const std::string root = "dataOutput/";
        
        time_t rawtime;
        struct tm * timeinfo;
        time (&rawtime);
        timeinfo = localtime (&rawtime);
        
        char buf[128];
        if (strftime(buf, sizeof(buf), "%Y-%m-%d_%H_%M_%S_%Z", timeinfo) == 0) {
            std::cout << "Failed to compute strftime for getDataOutputFolder(). Will use the seconds since 00:00:00 UTC, January 1, 1970 instead." << std::endl;
            cachedDataOutputFolderPath = root + std::to_string(rawtime);
        }
        cachedDataOutputFolderPath = root + buf;
        
        // Make destination directory
        if (mkdir(cachedDataOutputFolderPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH /* default mkdir permissions are probably: user can read, write, execute; group and others can read and execute */ ) != 0) {
            perror("mkdir failed");
            puts("Exiting.");
            exit(1);
        }
    }
    return cachedDataOutputFolderPath;
}
void saveMatrixGivenStr(const cv::Mat& M, std::string& name/*image name output*/,
                        const cv::Ptr<cv::Formatted>& str /*matrix contents input*/) {
    name = M.empty() ? openFileWithUniqueName(getDataOutputFolder() + "/firstImage", ".png") : openFileWithUniqueName(getDataOutputFolder() + "/scaled", ".png");
    if (!M.empty()) {
        auto matName = openFileWithUniqueName(name + ".matrix", ".txt");
        std::ofstream(matName.c_str()) << str << std::endl;
    }
}
void matrixToString(const cv::Mat& M,
                    cv::Ptr<cv::Formatted>& str /*matrix contents output*/) {
    cv::Ptr<cv::Formatter> fmt = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
//    fmt->set64fPrecision(4);
//    fmt->set32fPrecision(4);
    fmt->set64fPrecision(16);
    fmt->set32fPrecision(8);
    str = fmt->format(M);
}
void saveMatrix(const cv::Mat& M, std::string& name/*image name output*/,
                cv::Ptr<cv::Formatted>& str /*matrix contents output*/) {
    matrixToString(M, str);
    saveMatrixGivenStr(M, name, str);
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

#ifdef USE_COMMAND_LINE_ARGS
CommandLineConfig cfg;

#include <type_traits>
#endif
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

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--folder-data-source") == 0) { // Get from folder (path is specified with --folder-data-source-path, otherwise the default is used) instead of camera
            src = std::make_unique<FolderDataSource>(argc, argv, skip); // Read folder determined by command-line arguments
            cfg.folderDataSource = true;
        }
        if (strcmp(argv[i], "--video-file-data-source") == 0) { // Get from video file (path is specified with --video-file-data-source-path, otherwise the default is used) instead of camera
            src = std::make_unique<VideoFileDataSource>(argc, argv); // Read file determined by command-line arguments
            cfg.videoFileDataSource = true;
        }
        else if (strcmp(argv[i], "--camera-test-only") == 0) {
            cfg.cameraTestOnly = true;
        }
        else if (strcmp(argv[i], "--image-capture-only") == 0) { // For not running SIFT
            cfg.imageCaptureOnly = true;
        }
        else if (strcmp(argv[i], "--image-file-output") == 0) { // [mainInteractive() only] Outputs to video, usually also instead of preview window
            cfg.imageFileOutput = true;
        }
        else if (strcmp(argv[i], "--sift-video-output") == 0) { // Outputs frames with SIFT keypoints, etc. rendered to video file
            cfg.siftVideoOutput = true;
        }
        else if (i+2 < argc && strcmp(argv[i], "--skip-image-indices") == 0) { // Ignore images with this index. Usage: `--skip-image-indices 1 2` to skip images 1 and 2 (0-based indices, end index is exclusive)
            cfg.skipImageIndices.push_back(std::make_pair(std::stoi(argv[i+1]), std::stoi(argv[i+2])));
            i += 2;
        }
        else if (strcmp(argv[i], "--save-first-image") == 0) { // Save the first image SIFT encounters. Counts as a flush towards the start of SIFT.
            cfg.saveFirstImage = true;
        }
        else if (strcmp(argv[i], "--no-preview-window") == 0) { // Explicitly disable preview window
            cfg.noPreviewWindow = true;
        }
        else if (i+1 < argc && strcmp(argv[i], "--save-every-n-seconds") == 0) { // [mainMission() only] Save video and transformation matrices every n (given) seconds. Set to -1 to disable flushing until SIFT exits.
            cfg.flushVideoOutputEveryNSeconds = std::stoi(argv[i+1]);
            i++;
        }
        else if (strcmp(argv[i], "--main-mission") == 0) {
            cfg.mainMission = true;
        }
        else if (strcmp(argv[i], "--main-mission-interactive") == 0) { // Is --main-mission, but waits forever after a frame is shown in the preview window
            cfg.mainMission = true;
            cfg.waitKeyForever = true;
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            cfg.verbose = true;
        }
        else if (i+1 < argc && strcmp(argv[i], "--sleep-before-running") == 0) {
            long long time = std::stoll(argv[i+1]); // Time in milliseconds
            // Special value: -1 means sleep default time
            if (time == -1) {
                time = 30 * 1000;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
            i++;
        }
        else if (i+1 < argc && strcmp(argv[i], "--subscale-driver-fd") == 0) { // For grabbing IMU data, SIFT requires a separate driver program writing to the file descriptor given.
            driverInput_fd = std::stoi(argv[i+1]);
            printf("driverInput_fd set to: %d\n", driverInput_fd);
            i++;
        }
        else if (strcmp(argv[i], "--debug-no-std-terminate") == 0) { // Disables the SIFT unhandled exception handler for C++ exceptions. Use for lldb purposes.
            cfg.useSetTerminate = false;
        }
#ifdef SIFTAnatomy_
        else if (i+1 < argc && strcmp(argv[i], "--sift-params") == 0) {
            for (int j = i+1; j < argc; j++) {
#define LOG_PARAM(x) std::cout << "Set " #x << " to " << p.params->x << std::endl
                if (j+1 < argc && strcmp(argv[j], "-n_oct") == 0) {
                    p.params->n_oct = std::stoi(argv[j+1]);
                    LOG_PARAM(n_oct);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_spo") == 0) {
                    p.params->n_spo = std::stoi(argv[j+1]);
                    LOG_PARAM(n_spo);
                }
                else if (j+1 < argc && strcmp(argv[j], "-sigma_min") == 0) {
                    p.params->sigma_min = std::stof(argv[j+1]);
                    LOG_PARAM(sigma_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-delta_min") == 0) {
                    p.params->delta_min = std::stof(argv[j+1]);
                    LOG_PARAM(delta_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-sigma_in") == 0) {
                    p.params->sigma_in = std::stof(argv[j+1]);
                    LOG_PARAM(sigma_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-C_DoG") == 0) {
                    p.params->C_DoG = std::stof(argv[j+1]);
                    LOG_PARAM(sigma_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-C_edge") == 0) {
                    p.params->C_edge = std::stof(argv[j+1]);
                    LOG_PARAM(C_edge);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_bins") == 0) {
                    p.params->n_bins = std::stoi(argv[j+1]);
                    LOG_PARAM(n_bins);
                }
                else if (j+1 < argc && strcmp(argv[j], "-lambda_ori") == 0) {
                    p.params->lambda_ori = std::stof(argv[j+1]);
                    LOG_PARAM(lambda_ori);
                }
                else if (j+1 < argc && strcmp(argv[j], "-t") == 0) {
                    p.params->t = std::stof(argv[j+1]);
                    LOG_PARAM(t);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_hist") == 0) {
                    p.params->n_hist = std::stoi(argv[j+1]);
                    LOG_PARAM(n_hist);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_ori") == 0) {
                    p.params->n_ori = std::stoi(argv[j+1]);
                    LOG_PARAM(n_ori);
                }
                else if (j+1 < argc && strcmp(argv[j], "-lambda_descr") == 0) {
                    p.params->lambda_descr = std::stof(argv[j+1]);
                    LOG_PARAM(lambda_descr);
                }
                else if (j+1 < argc && strcmp(argv[j], "-itermax") == 0) {
                    p.params->itermax = std::stoi(argv[j+1]);
                    LOG_PARAM(itermax);
                }
                else {
                    // Continue with these parameters as the next arguments
                    i = j-1; // It will increment next iteration, so -1.
                    break;
                }
                j++; // Skip the number we parsed
            }
        }
        else if (i+1 < argc && strcmp(argv[i], "--sift-params-func") == 0) {
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
#endif
        else {
            // Whitelist: allow these to go through since they can be passed to DataSources, etc. //
            if (strcmp(argv[i], "--video-file-data-source-path") == 0) {
                i++; // Also go past the extra argument for this argument
                continue;
            }
            else if (strcmp(argv[i], "--read-backwards") == 0) {
                continue;
            }
            else if (strcmp(argv[i], "--crop-for-fisheye-camera") == 0) {
                continue;
            }
            else if (strcmp(argv[i], "--fps") == 0) {
                i++; // Also go past the extra argument for this argument
                continue;
            }
            // //
            
            printf("Unrecognized command-line argument given: %s", argv[i]);
            printf(" (command line was:\n");
            for (int i = 0; i < argc; i++) {
                printf("\t%s\n", argv[i]);
            }
            printf(")\n");
            puts("Exiting.");
            return 1;
        }
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

// Signal handling //
std::atomic<bool> g_stop;
void stopMain() {
    g_stop = true;
}
bool stoppedMain() {
    return g_stop;
}

// Prints a stacktrace
//void logTrace() {
//    using namespace backward;
//    StackTrace st; st.load_here();
//    Printer p;
//    p.print(st, stderr);
//}
void ctrlC(int s, siginfo_t *si, void *arg){
    printf_("Caught signal %d. Threads are stopping...\n",s); // Can't call printf because it might call malloc but the ctrlC might have happened in the middle of a malloc or free call, leaving data structures for it invalid. So we use `printf_` from https://github.com/mpaland/printf which is thread-safe and malloc-free but slow because it just write()'s all the characters one by one. Generally, "the signal handler code must not call non-reentrant functions that modify the global program data underneath the hood." ( https://www.delftstack.com/howto/c/sigint-in-c/ )
    // Notes:
    /* https://stackoverflow.com/questions/11487900/best-pratice-to-free-allocated-memory-on-sigint :
     Handling SIGINT: I can think of four common ways to handle a SIGINT:

     Use the default handler. Highly recommended, this requires the least amount of code and is unlikely to result in any abnormal program behavior. Your program will simply exit.

     Use longjmp to exit immediately. This is for folk who like to ride fast motorcycles without helmets. It's like playing Russian roulette with library calls. Not recommended.

     Set a flag, and interrupt the main loop's pselect/ppoll. This is a pain to get right, since you have to twiddle with signal masks. You want to interrupt pselect/ppoll only, and not non-reentrant functions like malloc or free, so you have to be very careful with things like signal masks. Not recommended. You have to use pselect/ppoll instead of select/poll, because the "p" versions can set the signal mask atomically. If you use select or poll, a signal can arrive after you check the flag but before you call select/poll, this is bad.

     Create a pipe to communicate between the main thread and the signal handler. Always include this pipe in the call to select/poll. The signal handler simply writes one byte to the pipe, and if the main loop successfully reads one byte from the other end, then it exits cleanly. Highly recommended. You can also have the signal handler uninstall itself, so an impatient user can hit CTRL+C twice to get an immediate exit.
     */
    stopMain();
    
    // Print stack trace
    //backward::sh->handleSignal(s, si, arg);
}
DataOutputBase* g_o2 = nullptr;
#undef BACKTRACE_SET_TERMINATE
#include "tools/backtrace/backtrace_on_terminate.hpp"
void terminate_handler(bool backtrace) {
    if (g_o2) {
        // [!] Take a risk and do something that can't always succeed in a segfault handler: possible malloc and state mutation:
        ((FileDataOutput*)g_o2)->release();
    
//        ((FileDataOutput*)g_o2)->writer.release(); // Save the file
//                    std::cout << "Saved the video" << std::endl;
    }
    else {
        // [!] Take a risk and do something that can't always succeed in a segfault handler: possible malloc and state mutation:
        std::cout << "No video to save" << std::endl;
    }
    
    if (backtrace) {
        backtrace_on_terminate();
    }
}
void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    printf_("Caught segfault (%s) at address %p. Running terminate_handler().\n", strsignal(signal), si->si_addr);
    
    terminate_handler(false);
    
    // Print stack trace
    //backward::sh->handleSignal(signal, si, arg);
    backtrace(std::cout);
    
    exit(5); // Probably doesn't call atexit since signal handlers don't.
}
// Installs signal handlers for the current thread.
void installSignalHandlers() {
    if (CMD_CONFIG(useSetTerminate)) {
        // https://en.cppreference.com/w/cpp/error/set_terminate
        std::set_terminate([](){
            std::cout << "Unhandled exception detected by SIFT. Running terminate_handler()." << std::endl;
            terminate_handler(true);
            std::abort(); // https://en.cppreference.com/w/cpp/utility/program/abort
        });
    }
    
    // Install ctrl-c handler
    // https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
    struct sigaction sigIntHandler;
    sigIntHandler.sa_sigaction = ctrlC;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    // Install segfault handler
    // https://stackoverflow.com/questions/2350489/how-to-catch-segmentation-fault-in-linux
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL); // Segfault handler! Tries to save stuff ASAP, unlike sigIntHandler.
    
    // Install SIGBUS (?), SIGILL (illegal instruction), and EXC_BAD_ACCESS (similar to segfault but is for "A crash due to a memory access issue occurs when an app uses memory in an unexpected way. Memory access problems have numerous causes, such as dereferencing a pointer to an invalid memory address, writing to read-only memory, or jumping to an instruction at an invalid address. These crashes are most often identified by the EXC_BAD_ACCESS (SIGSEGV) or EXC_BAD_ACCESS (SIGBUS) exceptions" ( https://developer.apple.com/documentation/xcode/investigating-memory-access-crashes )) handlers
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
}
// //

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
void onMatcherFinishedMatching(ProcessedImage<SIFT_T>& img2, bool showInPreviewWindow, bool useIdentityMatrix = false) {
    std::cout << "Matcher thread: finished matching" << std::endl;
    
    // Accumulate homography
    if (!useIdentityMatrix) {
        cv::Mat M = img2.transformation.inv();
        cv::Ptr<cv::Formatted> str;
        matrixToString(M, str);
        std::cout << "Accumulating new matrix: " << str << std::endl;
        lastImageToFirstImageTransformationMutex.lock();
        if (lastImageToFirstImageTransformation.empty()) {
            lastImageToFirstImageTransformation = M;
        }
        else {
            lastImageToFirstImageTransformation *= M;
        }
        lastImageToFirstImageTransformationMutex.unlock();
    }
    
    if (showInPreviewWindow) {
        if (CMD_CONFIG(showPreviewWindow())) {
            #ifdef USE_COMMAND_LINE_ARGS
            //assert(!img2.canvas.empty());
            if (img2.canvas.empty()) {
                std::cout << "Empty canvas image, avoiding showing it on canvasesReadyQueue" << std::endl;
            }
            else {
                canvasesReadyQueue.enqueue(img2);
            }
            #endif
        }
    }
}
// Assumes processedImageQueue.mutex is locked already.
void matcherWaitForNonPlaceHolderImageNoLockOnlyWait(bool seekInsteadOfDequeue, const int& currentIndex, const char* extraDescription="") {
    while( processedImageQueue.count < 1 + (seekInsteadOfDequeue ? currentIndex : 0) ) // Wait until more images relative to the starting position (`currentIndex`) are in the queue. (Usually `currentCount` is 0 but if it is >= 1 then it is because we're peeking further.)
    {
//        if (stoppedMain()) {
//            pthread_mutex_unlock( &processedImageQueue.mutex );
//            // Stop matcher thread
//            pthread_exit(0);
//        }
        pthread_cond_wait( &processedImageQueue.condition, &processedImageQueue.mutex );
        std::cout << "Matcher thread: Unlocking for matcherWaitForImageNoLock 2" << extraDescription << std::endl;
        pthread_mutex_unlock( &processedImageQueue.mutex );
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Matcher thread: Locking for matcherWaitForImageNoLock 2" << extraDescription << std::endl;
        pthread_mutex_lock( &processedImageQueue.mutex );
    }
}
// Assumes processedImageQueue.mutex is locked already.
void matcherWaitForNonPlaceholderImageNoLock(bool seekInsteadOfDequeue, int& currentIndex /*input and output*/) {
    t.reset();
    while (true) {
        matcherWaitForNonPlaceHolderImageNoLockOnlyWait(seekInsteadOfDequeue, currentIndex);

        // Dequeue placeholder/null or non-null images as needed
        ProcessedImage<SIFT_T>& front = seekInsteadOfDequeue ? processedImageQueue.get(currentIndex) : processedImageQueue.front();
        currentIndex++;
        if (front.n < 4 /*"null" image*/) {
            if (seekInsteadOfDequeue) {
                // Peek is done already since we did currentIndex++, nothing to do here.
                std::cout << "Matcher thread: Seeked to null image with index " << currentIndex-1 << std::endl;
            }
            else {
                std::cout << "Matcher thread: Dequeued null image with index " << currentIndex-1 << std::endl;
                // Dequeue (till non-null using the loop containing these statements)
                ProcessedImage<SIFT_T> placeholder;
                // Prevents waiting forever if last image we process is null //
//                matcherWaitForNonPlaceHolderImageNoLockOnlyWait(seekInsteadOfDequeue, currentIndex, ": dequeue till non-null");
                // //
                processedImageQueue.dequeueNoLock(&placeholder);
                if (CMD_CONFIG(showPreviewWindow()) && !seekInsteadOfDequeue) {
                    #ifdef USE_COMMAND_LINE_ARGS
                    assert(!placeholder.image.empty());
                    pthread_mutex_unlock( &processedImageQueue.mutex ); // Don't be "greedy"
                    canvasesReadyQueue.enqueueNoLock(placeholder);
                    pthread_mutex_lock( &processedImageQueue.mutex );
                    #endif
                }
            }
        }
        else {
            std::cout << "Matcher thread: Found non-null image with index " << currentIndex-1 << std::endl;
            // Dequeue and save the image outside this function, since we found it
            break; // `currentIndex` now points to the image we found that is non-null
        }
    }
    t.logElapsed("matcherWaitForNonPlaceholderImageNoLock");
}
int matcherWaitForTwoImages(ProcessedImage<SIFT_T>* img1 /*output*/, ProcessedImage<SIFT_T>* img2 /*output*/) {
    std::cout << "Matcher thread: Locking for 2x matcherWaitForImageNoLock" << std::endl;
    pthread_mutex_lock( &processedImageQueue.mutex );
    int currentIndex = 0;
    std::cout << "matcherWaitForNonPlaceholderImageNoLock: first image" << std::endl;
    matcherWaitForNonPlaceholderImageNoLock(false, currentIndex); // Dequeue a bunch of null images
    
    // Save the image from `matcherWaitForNonPlaceholderImageNoLock`
    processedImageQueue.dequeueNoLock(img1);
    //*img = processedImageQueue.content[processedImageQueue.readPtr + iMin];
    
    currentIndex = 0;
    std::cout << "matcherWaitForNonPlaceholderImageNoLock: second image" << std::endl;
    matcherWaitForNonPlaceholderImageNoLock(false, currentIndex); // Dequeue a bunch of null images
    
    // Save the image from `matcherWaitForNonPlaceholderImageNoLock` but with a peek instead of dequeue since the next matching round requires img2 as its img1
    processedImageQueue.peekNoLock(img2);
    
    pthread_mutex_unlock( &processedImageQueue.mutex );
    std::cout << "Matcher thread: Unlocked for 2x matcherWaitForImageNoLock" << std::endl;
    
    return currentIndex;
}
void* matcherThreadFunc(void* arg) {
    installSignalHandlers();
    
    #ifdef __APPLE__
    // https://stackoverflow.com/questions/2369738/how-to-set-the-name-of-a-thread-in-linux-pthreads
    // "Mac OS X: must be set from within the thread (can't specify thread ID)"
    pthread_setname_np("matcherThread");
    #endif
    
    SIFTState* s = (SIFTState*)arg;
    
    do {
//        OPTICK_THREAD("Worker");
        
        ProcessedImage<SIFT_T> img1, img2;
        int currentIndex = matcherWaitForTwoImages(&img1, &img2);
        
        // Setting current parameters for matching
        img2.applyDefaultMatchingParams();

        // Matching and homography
        t.reset();
        MatchResult enoughDescriptors = sift.findHomography(img1, img2, *s // Writes to img2.transformation
#ifdef USE_COMMAND_LINE_ARGS
                            , g_src, cfg
#endif
                            );
        t.logElapsed("find homography for second image");
        assert(enoughDescriptors != MatchResult::NotEnoughKeypoints); // This should be handled beforehand in the SIFT threads
        // If not enough for second image, we can match with a third.
        if (enoughDescriptors == MatchResult::NotEnoughMatchesForSecondImage) {
            std::cout << "Matcher thread: Not enough descriptors for second image detected. Retrying on third image." << std::endl;
            
            // Usually, SIFT threads can predict this by too low numbers of keypoints. 50 keypoints minimum, raises parameters over time beforehand.
            // Retry matching with the next image that comes in. If this fails, discard the last two images and continue.
            ProcessedImage<SIFT_T> img3;
            
            std::cout << "Matcher thread: Locking for peek third image" << std::endl;
            pthread_mutex_lock( &processedImageQueue.mutex );
            matcherWaitForNonPlaceholderImageNoLock(true, currentIndex); // Peek a bunch of null images
            pthread_mutex_unlock( &processedImageQueue.mutex );
            std::cout << "Matcher thread: Unlocking for peek third image" << std::endl;

            t.reset();
            MatchResult enoughDescriptors2 = sift.findHomography(img1, img3, *s // Writes to img3.transformation
            #ifdef USE_COMMAND_LINE_ARGS
                                        , g_src, cfg
            #endif
                                );
            t.logElapsed("find homography for third image");
            
            if (enoughDescriptors2 != MatchResult::Success) {
                std::cout << "Not enough descriptors the second time around" << std::endl;
            }
            else if (enoughDescriptors2 != MatchResult::NotEnoughKeypoints) {
                std::cout << "Got enough descriptors the second time around" << std::endl;
            }

            onMatcherFinishedMatching(img3, false, enoughDescriptors2 != MatchResult::Success);
            
            // Free memory and mark this as an unused ProcessedImage (optional):
            img1.resetMatching();
            img2.resetMatching();
            img3.resetMatching();
            
            // Regardless of enoughDescriptors2, we still continue with next iteration.
        }
        else if (enoughDescriptors == MatchResult::NotEnoughMatchesForFirstImage) {
            // Nothing we can do with the third image since this first image is the problem, so we skip this image
            // (Use identity matrix for this transformation)
            onMatcherFinishedMatching(img2, true, true);

            // Free memory and mark this as an unused ProcessedImage (optional):
            img1.resetMatching();
            img2.resetMatching();
        }
        else {
            // Save this good match
            onMatcherFinishedMatching(img2, true, false);
            
            // Free memory and mark this as an unused ProcessedImage (optional):
            img1.resetMatching();
            img2.resetMatching();
        }
    } while (!stoppedMain());
    return (void*)0;
}

cv::Mat prepareCanvas(ProcessedImage<SIFT_T>& img) {
    cv::Mat c = img.canvas.empty() ? img.image : img.canvas;
    if (CMD_CONFIG(showPreviewWindow())) {
        // Draw the image index
        // https://stackoverflow.com/questions/46500066/how-to-put-a-text-in-an-image-in-opencv-c/46500123
        cv::putText(c, //target image
                    std::to_string(img.i), //text
                    cv::Point(10, 25), //top-left position
                    //cv::Point(10, c.rows / 2), //center position
                    cv::FONT_HERSHEY_DUPLEX,
                    1.0,
                    CV_RGB(118, 185, 0), //font color
                    2);
    }
    return c;
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
        std::cout << "C++ exception from python: " << e.what() << std::endl;
    }
}

#define tpPush(x, ...) tp.push(x, __VA_ARGS__)
//#define tpPush(x, ...) x(-1, __VA_ARGS__) // Single-threaded hack to get exceptions to show! Somehow std::future can report exceptions but something needs to be done and I don't know what; see https://www.ibm.com/docs/en/i/7.4?topic=ssw_ibm_i_74/apis/concep30.htm and https://stackoverflow.com/questions/15189750/catching-exceptions-with-pthreads and `ctpl_stl.hpp`'s strange `auto push(F && f) ->std::future<decltype(f(0))>` function
//ctpl::thread_pool tp(4); // Number of threads in the pool
//ctpl::thread_pool tp(8);
ctpl::thread_pool tp(6);
// ^^ Note: "the destructor waits for all the functions in the queue to be finished" (or call .stop())
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
    std::cout << "Target fps: " << fps << std::endl;
    //std::atomic<size_t> offset = 0; // Moves back the indices shown to SIFT threads
    for (size_t i = src->currentIndex; !stoppedMain(); i++) {
//        OPTICK_FRAME("MainThread"); // https://github.com/bombomby/optick
        
        std::cout << "i: " << i << std::endl;
        #ifdef USE_COMMAND_LINE_ARGS
        for (auto& pair : cfg.skipImageIndices) {
            if (pair.first == i) {
                std::cout << "Skipping to index: " << pair.second << std::endl;
                for (size_t i2 = pair.first; i2 <= pair.second; i2++) {
                    cv::Mat mat = src->get(i2); // Consume images
                    // Enqueue null image
                    processedImageQueue.enqueue(mat,
                                            shared_keypoints_ptr(),
                                            std::shared_ptr<struct sift_keypoint_std>(),
                                            0,
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            shared_keypoints_ptr(),
                                            cv::Mat(),
                                            p,
                                            i2);
                }
                i = pair.second - 1; // -1 due to the i++
                continue;
            }
        }
        #endif
        auto sinceLast_milliseconds = since(last).count();
        if (src->wantsCustomFPS()) {
            if (sinceLast_milliseconds < timeBetweenFrames_milliseconds) {
                // Sleep
                auto sleepTime = timeBetweenFrames_milliseconds - sinceLast_milliseconds;
                std::cout << "Sleeping for " << sleepTime << " milliseconds" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }
        last = std::chrono::steady_clock::now();
        if (sinceLast_milliseconds > timeBetweenFrames_milliseconds) {
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
        std::cout << "CAP_PROP_POS_MSEC: " << src->timeMilliseconds() << std::endl;
        t.logElapsed("get image");
        if (mat.empty()) {
            printf("No more images left to process. Exiting.\n");
            stopMain();
            break;
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
                    std::cout << "Flushing with lag " << millis.count() - destMillis << " milliseconds" << std::endl;
                    
                    // Flush the matrix too //
                    lastImageToFirstImageTransformationMutex.lock();
                    if (!lastImageToFirstImageTransformation.empty() || CMD_CONFIG(saveFirstImage)) {
                        // Print the intermediate homography matrix
                        cv::Mat& M = lastImageToFirstImageTransformation;
                        cv::Ptr<cv::Formatted> str;
                        if (CMD_CONFIG(saveFirstImage)) {
                            std::cout << "Flushing first image" << std::endl;
                        }
                        else {
                            matrixToString(M, str);
                            std::cout << "Flushing homography: " << str << std::endl;
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
                        std::cout << "Saving to " << name << std::endl;
                        cv::imwrite(name, canvas);
#endif
                    }
                    else {
                        std::cout << "Homography not flushed since it is empty" << std::endl;
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

        printf("Image %zu: Blur detection: variance %f; ", i, variance);
        if (variance <= threshold) {
            // Blurry
            puts("blurry");
            discardImage = true;
        } else {
            // Not blurry
            puts("not blurry");
        }
        t.logElapsed("blur detection");
        
        // IMU
        if (driverInput_fd != -1) { //driverInput_file) {
            IMUData imu;
            // NOTE: this blocks until the fd gets data.
            t.reset();
            std::cout << "grabbing from subscale driver interface thread..." << std::endl;
            subscaleDriverInterfaceMutex_imuData.lock();
            int count = subscaleDriverInterface_count;
            static bool firstGrab = true;
            static py::object my_or;
            if (count > 0) {
                // We have something to use
                IMUData& imu = subscaleDriverInterface_imu;
                std::cout << "Linear accel NED: " << imu.linearAccelNed << " (data rows read: " << count << ")" << std::endl;
                
                // Use the IMU data:
                nonthrowing_python([](){
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
                        std::cout << "Python likes this image" << std::endl;
                    }
                    else {
                        std::cout << "Python doesn't like this image" << std::endl;
                        discardImage = true;
                    }
                });
            }
            subscaleDriverInterfaceMutex_imuData.unlock();
            t.logElapsed("grabbing from subscale driver interface thread and running Precession.judge_image");
        }
        // //
        
        if (discardImage) {
            // Enqueue null image
            processedImageQueue.enqueue(greyscale,
                                    shared_keypoints_ptr(),
                                    std::shared_ptr<struct sift_keypoint_std>(),
                                    0,
                                    shared_keypoints_ptr(),
                                    shared_keypoints_ptr(),
                                    shared_keypoints_ptr(),
                                    cv::Mat(),
                                    p,
                                    i);
            
            // Don't push a function for this image or save it as a possible firstImage
            goto skipImage;
        }
        std::cout << "Pushing function to thread pool, currently has " << tp.n_idle() << " idle thread(s) and " << tp.q.size() << " function(s) queued" << std::endl;
        tpPush([&pOrig=p](int id, /*extra args:*/ size_t i, cv::Mat greyscale) {
//            OPTICK_EVENT();
            std::cout << "hello from " << id << std::endl;
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
            std::cout << id << " findKeypoints" << std::endl;
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
            std::cout << id << " findKeypoints end" << std::endl;
            
#ifdef USE_PTR_INC_MALLOC
            endMallocWithFreeAll();
#endif
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
                    || (processedImageQueue.readPtr == (i - 1) % processedImageQueueBufferSize) // If we're writing right after the last element, can enqueue our sequentially ordered image
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
            // Log malloc, etc. timers and reset them
            showTimers(std::string("SIFT ") + std::to_string(id));
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
        
        skipImage:
#ifdef USE_COMMAND_LINE_ARGS
        // Show images if we have them and if we are showing a preview window
        if (!canvasesReadyQueue.empty() && CMD_CONFIG(showPreviewWindow())) {
            ProcessedImage<SIFT_T> img;
            canvasesReadyQueue.dequeue(&img);
            cv::Mat realCanvas = prepareCanvas(img);
            commonUtils::imshow("", realCanvas); // Canvas can be empty if no matches were done on the image, hence nothing was rendered. // TODO: There may be some keypoints but we don't show them..
            if (CMD_CONFIG(siftVideoOutput)) {
                // Save frame with SIFT keypoints rendered on it to the video output file
                cv::Rect rect = src->shouldCrop() ? src->crop() : cv::Rect();
                if (img.canvas.empty()) {
                    std::cout << "Canvas has empty image, using original" << std::endl;
                    o2.showCanvas("", img.image, false, rect.empty() ? nullptr : &rect);
                }
                else {
                    o2.showCanvas("", img.canvas, false, rect.empty() ? nullptr : &rect);
                }
            }
            //cv::waitKey(30);
            auto size = canvasesReadyQueue.size();
            std::cout << "Showing image from canvasesReadyQueue with " << size << " images left" << std::endl;
            char c = cv::waitKey(CMD_CONFIG(waitKeyForever) ? 0 : (1 + 150.0 / (1 + size))); // Sleep less as more come in
            if (c == 'q') {
                // Quit
                std::cout << "Exiting (q pressed)" << std::endl;
                stopMain();
            }
        }
#endif
        
        showTimers("main");
    }
    
    o2.release(); // Save the file
    
    // Show the rest of the images only if we stopped because we finished reading all frames and if we are showing a preview window
    if (!stoppedMain() && CMD_CONFIG(showPreviewWindow())) {
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
            cv::Mat realCanvas = prepareCanvas(img);
            commonUtils::imshow("", realCanvas);
            if (CMD_CONFIG(siftVideoOutput)) {
                // Save frame with SIFT keypoints rendered on it to the video output file
                cv::Rect rect = src->shouldCrop() ? src->crop() : cv::Rect();
                o2.showCanvas("", realCanvas, false, rect.empty() ? nullptr : &rect);
            }
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
    
    std::cout << "Broadcasting stoppedMain()" << std::endl;
    
    // Wake up matcher thread and other SIFT threads so they see that we stoppedMain()
//    pthread_cond_broadcast(&processedImageQueue.condition); // Note: "The signal and broadcast operations do not require a mutex. A condition variable also is not permanently associated with a specific mutex; the external mutex does not protect the condition variable." ( https://stackoverflow.com/questions/2763714/why-do-pthreads-condition-variable-functions-require-a-mutex#:~:text=The%20signal%20and%20broadcast%20operations,not%20protect%20the%20condition%20variable. ) + verified by POSIX spec: "The pthread_cond_broadcast() or pthread_cond_signal() functions may be called by a thread whether or not it currently owns the mutex that threads calling pthread_cond_wait() or pthread_cond_timedwait() have associated with the condition variable during their waits; however, if predictable scheduling behavior is required, then that mutex shall be locked by the thread calling pthread_cond_broadcast() or pthread_cond_signal()." ( https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_broadcast.html )
    
    std::cout << "Stopping thread pool" << std::endl;
    
    tp.stop(!stoppedMain() /*true to wait for queued functions as well*/); // Join thread pool's threads
    
    std::cout << "Cancelling matcher thread" << std::endl;
    
    // Sometimes, the matcher thread is waiting on the condition variable still, and it will be waiting forever unless we interrupt it somehow (or broadcast on the condition variable..).
    pthread_cancel(matcherThread); // https://man7.org/linux/man-pages/man3/pthread_cancel.3.html
    if (driverInput_fd != -1) {
        std::cout << "Joining subscaleDriverInterfaceThread" << std::endl;
        pthread_cancel(subscaleDriverInterfaceThread);
    }
    
    std::cout << "Joining matcher thread" << std::endl;
    
    int ret1;
    pthread_join(matcherThread, (void**)&ret1);
    if (driverInput_fd != -1) {
        std::cout << "Joining subscaleDriverInterfaceThread" << std::endl;
        int ret2;
        pthread_join(subscaleDriverInterfaceThread, (void**)&ret2);
    }
    
    // Print the final homography
    //[unnecessary since matcherThread is stopped now:] lastImageToFirstImageTransformationMutex.lock();
    cv::Mat& M = lastImageToFirstImageTransformation;
    cv::Ptr<cv::Formatted> str;
    matrixToString(M, str);
    std::cout << "Final homography: " << str << std::endl;
    
    // Save final homography matrix
    std::string name;
    saveMatrixGivenStr(M, name /* <--output */, str);
    //lastImageToFirstImageTransformationMutex.unlock();
    
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
    bool crop;
    crop = src->shouldCrop();
    //crop = false; // Hardcode, since it is weird when you scale the cropped image?
    cv::Mat img = crop ? firstImage(src->crop()) : firstImage;
    cv::warpPerspective(img, canvas /* <-- destination */, M, img.size());
//    cv::warpAffine(firstImage, canvas /* <-- destination */, M, firstImage.size());
    std::cout << "Saving to " << name << std::endl;
    cv::imwrite(name, canvas);
    return 0;
}

#ifdef USE_COMMAND_LINE_ARGS
#ifdef SIFTAnatomy_
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
        o2.release(); // Save the file
        #endif
    }
    
    return 0;
}
#endif
#endif // USE_COMMAND_LINE_ARGS
