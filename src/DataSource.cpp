//
//  DataSource.cpp
//  SIFT
//
//  Created by VADL on 11/2/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "DataSource.hpp"

#include <filesystem>
namespace fs = std::filesystem;
//namespace fs = std::__fs::filesystem;
#include "strnatcmp.hpp"

#include "utils.hpp"

#include "common.hpp"

#include "../sharedMemory/sharedMemory.hpp"
#include "../sharedMemory/SharedMemoryConvention.hpp"

template <typename F1, typename F2>
cv::Mat siftImageForMatFromColoredImageInCache(size_t index, std::unordered_map<size_t, cv::Mat>& cache, F1 shouldCrop, F2 crop);

cv::Rect cropForFisheyeCamera() {
    //return cv::Rect(cv::Point(137,62), cv::Point(490,401));
    int inset;
    //inset = 60;
    inset = 0;
    return cv::Rect(cv::Point(137+inset,62+inset), cv::Point(490-inset,401-inset));
}
void DataSourceBase::initCrop(cv::Size sizeFrame) {
    _cropRect = cropForFisheyeCamera(); // Only used if _crop is true
    #ifdef SIFTAnatomy_
    // SIFT Anatomy can only handle square images, so make a center crop if one wasn't done yet:
    if (!_crop && sizeFrame.width > sizeFrame.height) {
        int excess = sizeFrame.width - sizeFrame.height;
        _cropRect.x = excess / 2;
        _cropRect.width = sizeFrame.width - excess / 2;
        _cropRect.y = 0;
        _cropRect.height = sizeFrame.height;
        std::cout << "Auto-cropping to centered square, crop is: " << _cropRect << std::endl;
        _crop = true;
    }
    #endif
}

StreamDataSource::StreamDataSource(std::function<cv::Mat(void)> getNextInStream_) {
    currentIndex = 0;
    getNextInStream = getNextInStream_;
}

bool StreamDataSource::hasNext() {
    return true;
}

cv::Mat StreamDataSource::next() {
    return get(currentIndex++);
}

static thread_local std::string msg;
cv::Mat StreamDataSource::get(size_t index) {
    size_t i = index;
    
//        if (!(i < files.size())) {
//            return cv::Mat(); // End is reached, no more images
//        }
    
    if (!inittedCrop) {
        // Ensure we init crop
        inittedCrop = true;
        cv::Mat res = get(0);
        sizeFrame.width = res.cols;
        sizeFrame.height = res.rows;
        initCrop(sizeFrame);
    }
    auto it = cache.find(i);
    if (it != cache.end()) {
        return it->second;
    }

//    std::string msg2 = "Testing:";
//    msg2 += " (index was " + std::to_string(i) + " but currentIndex was " + std::to_string(currentIndex) + ")";
//    std::cout << msg2 << std::endl;
    
    if (i < currentIndex) {
        msg = "Index given has no cached image";
        msg += " (index was " + std::to_string(i) + " but currentIndex was " + std::to_string(currentIndex) + ")";
        recoverableError(msg.c_str());
        return cv::Mat();
    }
    else if (i == currentIndex) {
        currentIndex++;
    }
    else if (i > currentIndex) { // Index given is too large
        return cv::Mat();
    }

    // Loading image
    t.reset();
    cv::Mat mat = getNextInStream();
    t.logElapsed("StreamDataSource load image from getNextInStream()");
    
    // Ensure good size for SIFT
//    if (w > sizeFrame.width || h > sizeFrame.height) {
//        t.reset();
//
//        // TODO: Keep same aspect ratio
////        double EPSILON = 0.0001;
////        if (sizeFrame.width / (double)sizeFrame.height - w / (double)h > EPSILON) {
////
////        }
//
//        { out_guard();
//            std::cout << "Resize from " << w << " x " << h << " to " << sizeFrame << std::endl; }
//        cv::resize(mat, mat, sizeFrame);
//        t.logElapsed("resize frame for SIFT");
//    }
    
    // Cache it
    cache.emplace(i, mat);
    
//    printf("TESTINGasd\n");
    
    return mat;
}

std::string StreamDataSource::nameForIndex(size_t index) {
    return "Image " + std::to_string(index);
}

cv::Mat StreamDataSource::siftImageForMat(size_t index) {
    return siftImageForMatFromColoredImageInCache(index, cache, std::bind(&OpenCVVideoCaptureDataSource::shouldCrop, *this), std::bind(&OpenCVVideoCaptureDataSource::crop, *this));
}
cv::Mat StreamDataSource::colorImageForMat(size_t index) {
    return cache.at(index);
}

// https://stackoverflow.com/questions/6537436/how-do-you-get-file-size-by-fd
//off_t fileSize(int fd) {
//   struct stat s;
//   if (fstat(fd, &s) == -1) {
//      int saveErrno = errno;
//      fprintf(stderr, "fstat(%d) returned errno=%d (%s).", fd, saveErrno, strerror(errno));
//      return(-1);
//   }
//   return(s.st_size);
//}

SharedMemoryDataSource::SharedMemoryDataSource(int fd) :
shmem(nullptr),
shmem_size(0),
shmemFD(fd),
StreamDataSource([this](){
    SharedMemoryConvention* shmem = ((SharedMemoryConvention*)this->shmem);
    if (shmem == nullptr) {
        puts("SharedMemoryDataSource: no shmem pointer was set");
        return cv::Mat();
    }
    // Get a cv::Mat from the shared memory
    lockSharedMemoryWithConvention(shmem);
    
    // Check if we have an image yet
    while (/* (This condition is included in the condition that followed it: `shmem->frameCounter == std::numeric_limits<uint64_t>::max() ||`) */
           shmem->frameCounter == this->lastShmemFrameCounter // On the same image, so wait for another
           ) {
        // Wait for an image
        unlockSharedMemoryWithConvention(shmem);
        puts("SharedMemoryDataSource: waiting for an image...");
        usleep(100 // <--milliseconds
               *1000); // (Microseconds after conversion)
        lockSharedMemoryWithConvention(shmem);
    }
    
    // Check for closed data source
    if (shmem->frameCounter == std::numeric_limits<uint64_t>::max() - 1) {
        puts("SharedMemoryDataSource: data source was closed from outside SIFT");
        unlockSharedMemoryWithConvention(shmem);
        return cv::Mat();
    }
    
    this->lastShmemFrameCounter = shmem->frameCounter;
    printf("SharedMemoryDataSource: shmem->frameCounter: " "%" PRIu64 "\n",  shmem->frameCounter);
    
    // size_t size = this->shmem+this->shmem_size-addr
    // Copy (so we can release the lock)
    cv::Mat mat(shmem->imgHeight_, shmem->imgWidth_, CV_8UC3); // Default-initialized matrix
    memcpy(mat.data, &shmem->mat, sizeof(shmem->mat));// Copy into our own Mat
    
    unlockSharedMemoryWithConvention(shmem);
    return mat;
})
{
//    this->shmem_size = fileSize(this->shmemFD);
//    if (this->shmem_size <= 0) {
//        return;
//    }
    this->shmem = (SharedMemoryConvention*)map_shared_memory_from_fd(this->shmemFD, &this->shmem_size);
    if (this->shmem == nullptr) {
        puts("SharedMemoryDataSource: failed to map_shared_memory_from_fd(). Ignoring.");
        return;
    }
    SharedMemoryConvention* shmem = ((SharedMemoryConvention*)this->shmem);
    assert(this->shmem_size == sizeof(SharedMemoryConvention)); // Ensure the other program's established shared memory is the size we know of.
    lockSharedMemoryWithConvention(shmem);
    //printf("SharedMemoryDataSource: mapped shared memory with message: %s\n", shmem->msg);
    printf("SharedMemoryDataSource: mapped shared memory with shmem_size: %zu, sizeof(SharedMemoryConvention): %zu, frameCounter: " "%" PRIu64 "\n" // https://stackoverflow.com/questions/9225567/how-to-print-a-int64-t-type-in-c
           , this->shmem_size, sizeof(SharedMemoryConvention), shmem->frameCounter);
    unlockSharedMemoryWithConvention(shmem);
}

FolderDataSource::FolderDataSource(std::string folderPath, size_t skip_ = 0) {
    currentIndex = skip_;
    init(folderPath);
}

// Parses size_t from a command-line argument
size_t getSize(char* arg) {
    // Use given skip (selects the first image to show)
    char* endptr;
    std::uintmax_t skip_ = std::strtoumax(arg, &endptr, 0); // https://en.cppreference.com/w/cpp/string/byte/strtoimax
    // Check for overflow
    auto max = std::numeric_limits<size_t>::max();
    if (skip_ > max || ERANGE == errno) {
        std::cout << "Argument (\"" << skip_ << "\") is too large for size_t (max value is " << max << "). Exiting." << std::endl;
        exit(1);
    }
    // Check for conversion failures ( https://wiki.sei.cmu.edu/confluence/display/c/ERR34-C.+Detect+errors+when+converting+a+string+to+a+number )
    else if (endptr == arg) {
        std::cout << "Argument (\"" << arg << "\") could not be converted to an unsigned integer. Exiting." << std::endl;
        exit(2);
    }
    return skip_;
}
template <typename T1, typename T2> T2 ensureFitsInT2(T1 val, std::function<void(T1)> showFailureMsg) {
    if (val < std::numeric_limits<T2>::max()) {
        return val;
    }
    showFailureMsg(val);
    exit(1);
}
cv::Size getSizeFrame(char* arg1, char* arg2) {
    std::function<void(size_t)> m = [](size_t val) {
        auto max = std::numeric_limits<int>::max();
        std::cout << "Argument (\"" << val << "\") is too large for int (max value is " << max << "). Exiting." << std::endl;
    };
    return {ensureFitsInT2<size_t, int>(getSize(arg1), m), ensureFitsInT2<size_t, int>(getSize(arg2), m)};
}
FolderDataSource::FolderDataSource(int argc, char** argv, size_t skip) {
    currentIndex = skip;
    sizeFrame = CameraDataSource::default_sizeFrame;
    
    // Set the default folder path
    std::string folderPath = "testFrames2_cropped"; //"testFrames1";
    //std::string folderPath = "outFrames";
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (i+1 < argc) {
            if (strcmp(argv[i], "--folder-data-source-path") == 0) {
                folderPath = argv[i+1];
                i++;
            }
            else if (strcmp(argv[i], "--skip") == 0) {
                skip = getSize(argv[i+1]);
                printf("Using skip %zu\n", skip); // Note: if you give a negative number: "If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type." ( https://en.cppreference.com/w/c/string/byte/strtoimax )
                i++;
            }
            else if (i+2 < argc && strcmp(argv[i], "--size-frame") == 0) {
                sizeFrame = getSizeFrame(argv[i+1], argv[i+2]);
                std::cout << "Using size " << sizeFrame << '\n';
                i += 2;
            }
            else if (strcmp(argv[i], "--crop-for-fisheye-camera") == 0) {
                std::cout << "Using crop for fisheye camera." << '\n';
                _crop = true;
                i++;
            }
//            else {
//                goto else_;
//            }
        }
//        else {
//else_:
//            if (argc > 1) {
//                skip = getSkip(argv[1]);
//            }
//        }
    }
    
    init(folderPath);
}

void FolderDataSource::init(std::string folderPath) {
    // For each output image, loop through it
    // https://stackoverflow.com/questions/62409409/how-to-make-stdfilesystemdirectory-iterator-to-list-filenames-in-order
    //--- filenames are unique so we can use a set
    //std::vector<std::string> files;
    for (const auto & entry : fs::directory_iterator(folderPath)) {
        // Ignore files we treat specially:
        if (entry.is_directory() || endsWith(entry.path().string(), ".keypoints.txt") || endsWith(entry.path().string(), ".matches.txt") || entry.path().filename().string() == ".DS_Store") {
            continue;
        }
        files.push_back(entry.path().string());
    }

    // https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
    std::sort(files.begin(),files.end(),compareNat);
}

FolderDataSource::~FolderDataSource() {
    for (auto i : cache) {
        free(i.second.data);
    }
}

bool FolderDataSource::hasNext() {
    return currentIndex < files.size();
}

cv::Mat FolderDataSource::next() {
    return get(currentIndex++);
}

cv::Mat FolderDataSource::get(size_t index) {
    size_t i = index;
    //for (size_t i = skip; i < files.size(); i++) {
        //for (size_t i = files.size() - 1 - skip; i < files.size() /*underflow of i will end the loop*/; i--) {
    if (!(i < files.size())) {
        return cv::Mat(); // End is reached, no more images
    }
    
    auto& path = files[i];
    std::cout << path << std::endl;
    
    if (!inittedCrop) {
        // Ensure we init crop
        inittedCrop = true;
        cv::Mat res = get(0);
        sizeFrame.width = res.cols;
        sizeFrame.height = res.rows;
        initCrop(sizeFrame);
    }
    auto it = cache.find(i);
    if (it != cache.end()) {
        return it->second;
    }

    // Loading image
    t.reset();
    size_t w, h;
    errno = ESUCCESS; // Set errno so we can read it again to check why any errors happen after the below call
    float* x = io_png_read_f32_gray(path.c_str(), &w, &h);
    t.logElapsed("load image");
    if (x == nullptr) {
        if (errno != ESUCCESS) {
            perror("");
            fatal_error("File \"%s\" could not be opened", path.c_str());
        }
        else {
            fatal_error("File \"%s\" cannot be loaded as an image", path.c_str());
        }
    }
    t.reset();
    for(int i=0; i < w*h; i++)
        x[i] /=256.; // TODO: why do we do this?
    t.logElapsed("normalize image");
    
    // Make OpenCV matrix with no copy ( https://stackoverflow.com/questions/44453088/how-to-convert-c-array-to-opencv-mat )
    cv::Mat mat(h, w, CV_32F, x); // Is black and white
    
    // Ensure good size for SIFT
    if (w > sizeFrame.width || h > sizeFrame.height) {
        t.reset();
        
        // TODO: Keep same aspect ratio
//        double EPSILON = 0.0001;
//        if (sizeFrame.width / (double)sizeFrame.height - w / (double)h > EPSILON) {
//
//        }
        
        { out_guard();
            std::cout << "Resize from " << w << " x " << h << " to " << sizeFrame << std::endl; }
        cv::resize(mat, mat, sizeFrame);
        t.logElapsed("resize frame for SIFT");
    }
    
    // Cache it
    cache.emplace(i, mat);
    
    return mat;
}

std::string FolderDataSource::nameForIndex(size_t index) {
    return files[index];
}

cv::Mat FolderDataSource::siftImageForMat(size_t index) {
    return cache.at(index);
}
cv::Mat FolderDataSource::colorImageForMat(size_t index) {
    cv::Mat color = cache.at(index);
    
    // Make the black and white OpenCV matrix into color but still black and white (we do this so we can draw colored rectangles on it later)
    cv::Mat backtorgb;
    t.reset();
    cv::cvtColor(color, backtorgb, cv::COLOR_GRAY2RGBA); // https://stackoverflow.com/questions/21596281/how-does-one-convert-a-grayscale-image-to-rgb-in-opencv-python
    t.logElapsed("convert image to color");
    return backtorgb;
}
#define CROP_FOR_FISHEYE_CAMERA
cv::Mat OpenCVVideoCaptureDataSource::siftImageForMat(size_t index) {
    return siftImageForMatFromColoredImageInCache(index, cache, std::bind(&OpenCVVideoCaptureDataSource::shouldCrop, *this), std::bind(&OpenCVVideoCaptureDataSource::crop, *this)); // Have we defeated OOP once and for all?   // https://stackoverflow.com/questions/30930350/why-member-functions-cant-be-used-as-template-arguments
}
template <typename F1, typename F2>
cv::Mat siftImageForMatFromColoredImageInCache(size_t index, std::unordered_map<size_t, cv::Mat>& cache, F1 shouldCrop, F2 crop) {
    cv::Mat grey = cache.at(index);
    
    if (shouldCrop()) {
        cv::Mat cropped_image = grey(crop());
        grey = cropped_image;
    }
    
#ifdef SIFTAnatomy_
    cv::Mat mat;
    t.reset();
    { out_guard();
        std::cout << mat_type2str(grey.type()) << std::endl; }
    cv::cvtColor(grey, mat, cv::COLOR_RGB2GRAY);
    mat.convertTo(mat, CV_32F, 1/255.0); // https://stackoverflow.com/questions/22174002/why-does-opencvs-convertto-function-not-work : need to scale the values down to float image's range of 0-1););
    { out_guard();
        std::cout << mat_type2str(mat.type()) << std::endl; }
    t.logElapsed("convert image to greyscale and float");
    return mat;
#else
    return grey;
#endif
}
cv::Mat OpenCVVideoCaptureDataSource::colorImageForMat(size_t index) {
    return cache.at(index);
}

OpenCVVideoCaptureDataSource::OpenCVVideoCaptureDataSource() {
    currentIndex = 0;
}

bool DataSourceBase::shouldCrop() const {
    return _crop;
}
cv::Rect DataSourceBase::crop() const {
    if (shouldCrop()) {
        return _cropRect;
    }
    return cv::Rect();
}

const double OpenCVVideoCaptureDataSource::default_fps =
#ifdef USE_COMMAND_LINE_ARGS
3 //4 //30
// NOTE: FPS of 1 will make the camera really dark! Don't use it!
#else
3
// NOTE: FPS of 1 will make the camera really dark! Don't use it!
#endif
;
const cv::Size CameraDataSource::default_sizeFrame = {640,480};
//cv::Size sizeFrame(1920,1080);
CameraDataSource::CameraDataSource() {
    init(default_fps, default_sizeFrame);
}

CameraDataSource::CameraDataSource(int argc, char** argv) {
    double fps = default_fps;
    cv::Size sizeFrame = default_sizeFrame;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (i+1 < argc) {
            if (strcmp(argv[i], "--fps") == 0) {
                fps = std::stod(argv[i+1]);
                i++;
            }
            else if (i+2 < argc && strcmp(argv[i], "--size-frame") == 0) {
                sizeFrame = getSizeFrame(argv[i+1], argv[i+2]);
                std::cout << "Using size " << sizeFrame << '\n';
                i += 2;
            }
            else if (strcmp(argv[i], "--crop-for-fisheye-camera") == 0) {
                std::cout << "Using crop for fisheye camera." << '\n';
                _crop = true;
                i++;
            }
//            else {
//                goto else_;
//            }
        }
//        else {
//else_:
//            if (argc > 1) {
//                skip = getSkip(argv[1]);
//            }
//        }
    }

    init(fps, sizeFrame);
}

void CameraDataSource::init(double fps, cv::Size sizeFrame) {
    currentIndex = 0; // Index to save into next
    initCrop(sizeFrame); // Possibly overrides the crop set above
    
    // NOTE: FPS of 1 will make the camera really dark! Don't use it!
    assert(fps > 1);
    
    // Instanciate the capture
    cap.open(0);

    if (!cap.isOpened()) { // check if we succeeded
      std::cout << "Unable to start capture" << std::endl;
      throw 0;
    }

    // https://learnopencv.com/how-to-find-frame-rate-or-frames-per-second-fps-in-opencv-python-cpp/
    // "For OpenCV 3, you can also use the following"
    double fpsOrig = cap.get(cv::CAP_PROP_FPS);
    double widthOrig = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double heightOrig = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    cap.set(cv::CAP_PROP_FRAME_WIDTH, sizeFrame.width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, sizeFrame.height);
    cap.set(cv::CAP_PROP_FPS, fps);
    
    // Sometimes the requested values aren't supported, so they stay the same. If so, we have a separate fps value stored for that called `wantedFPS`.
    double fpsReported = cap.get(cv::CAP_PROP_FPS);
    double widthReported = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double heightReported = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    
    if (fpsReported != fps) {
        wantedFPS = fps;
    }
    
    std::cout << "Original frames per second using video.get(CAP_PROP_FPS) : " << fpsOrig << "\n\tOriginal width: " << widthOrig << "\n\tOriginal height: " << heightOrig
    << "\n\tSet fps to: " << fps << "\n\tSet width and height to: " << sizeFrame
    << "\n\tFinal reported fps: " << fpsReported << "\n\tFinal reported width: " << widthReported << "\n\tFinal reported height: " << heightReported << std::endl;

    
    //cv::Mat frame_;
    // int i = 0;
    // while ((i++ < 100) && !cap.read(frame_)) {
    // }; // warm up camera by skiping unread frames
}

bool CameraDataSource::hasNext() {
    return cap.isOpened(); // TODO: make this return whether we "sync"ed with the camera or not since the last grab. Use https://man7.org/linux/man-pages/man2/select.2.html and /Volumes/MyTestVolume/MiscRepos/opencv/modules/videoio/src/cap_v4l.cpp:996 to change timeout of select() system call to instant so it doesn't hang around waiting for data. Low level stuff explained: https://web.archive.org/web/20140401235613/http://pages.cpsc.ucalgary.ca/~sayles/VFL_HowTo/
    // https://www.tutorialspoint.com/unix_system_calls/_newselect.htm : "timeout is an upper bound on the amount of time elapsed before select() returns. It may be zero, causing select() to return immediately. (This is useful for polling.) If timeout is NULL (no timeout), select() can block indefinitely."
}

cv::Mat OpenCVVideoCaptureDataSource::next() {
    return get(currentIndex++);
}

cv::Mat OpenCVVideoCaptureDataSource::get(size_t index) {
    size_t i = index;
    
    auto it = cache.find(i);
    if (it != cache.end()) {
        return it->second;
    }
    
    if (i < currentIndex) {
        recoverableError("Index given has no cached image");
        return cv::Mat();
    }
    else if (i == currentIndex) {
        currentIndex++;
    }
    else if (i > currentIndex) { // Index given is too large
        return cv::Mat();
    }
    
    cv::Mat mat;
    if (!cap.read(mat)) {
        recoverableError("Error reading from camera");
        return mat;
    }
    
#ifndef USE_COMMAND_LINE_ARGS
    // Clean up cache a bit
    cache.clear();
#endif
    
    // Cache it
    cache.emplace(i, mat);

    return mat;
}

std::string OpenCVVideoCaptureDataSource::nameForIndex(size_t index) {
    return "Image " + std::to_string(index);
}

const std::string VideoFileDataSource::default_filePath = "quadcopterFlight/live2--.mp4";
VideoFileDataSource::VideoFileDataSource() {
    init(default_filePath);
}

VideoFileDataSource::VideoFileDataSource(int argc, char** argv) {
    // Parse arguments
    std::string filePath = default_filePath;
    bool ensureReadBackwards = false;
    for (int i = 1; i < argc; i++) {
        if (i+1 < argc && strcmp(argv[i], "--video-file-data-source-path") == 0) {
            filePath = argv[i+1];
            i++;
            readBackwards = false;
#define allowSeek readBackwards // Some weird thing happens on some videos it seems, where even seeking to the end of the video to get the length and then back to the start (in order to obey readBackwards == false) causes cap.read() to fail. This is a workaround.. to disable the workaround, define allowSeek as true.
        }
        else if (strcmp(argv[i], "--read-backwards") == 0) {
            ensureReadBackwards = true;
        }
        else if (strcmp(argv[i], "--crop-for-fisheye-camera") == 0) {
            std::cout << "Using crop for fisheye camera." << '\n';
            _crop = true;
            i++;
        }
    }
    if (ensureReadBackwards) {
        readBackwards = true;
    }
    
    init(filePath);
}

// https://stackoverflow.com/questions/11260042/reverse-video-playback-in-opencv
void VideoFileDataSource::init(std::string filePath) {
    // Instanciate the capture
    std::cout << "Opening " << filePath << std::endl;
    cap.open(filePath);
    
    double widthReported = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double heightReported = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    cv::Size sizeFrame(widthReported, heightReported);
    initCrop(sizeFrame);

    if (!cap.isOpened()) { // check if we succeeded
      std::cout << "Unable to open capture on given file: " << filePath << std::endl;
      throw 0;
    }

    double frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);
    frame_rate = cap.get(cv::CAP_PROP_FPS);
    
    if (frame_rate != default_fps) {
        wantedFPS = default_fps;
    }
    
    // Calculate number of msec per frame.
    // (msec/sec / frames/sec = msec/frame)
    frame_msec = 1000 / frame_rate;
    
    if (allowSeek) {
        // Seek to the end of the video.
        cap.set(cv::CAP_PROP_POS_AVI_RATIO, 1);
        
        // Get video length
        cap.set(cv::CAP_PROP_POS_MSEC, frame_msec * frame_count);
        video_time = cap.get(cv::CAP_PROP_POS_MSEC);
        
        if (!readBackwards) {
            cap.set(cv::CAP_PROP_POS_AVI_RATIO, 0);
            cap.set(cv::CAP_PROP_POS_MSEC, 0);
        }
    }
    
    std::cout << "Video duration: " << (allowSeek ? video_time : (frame_msec*frame_count)) << " milliseconds; frame count: " << frame_count << std::endl;
}

cv::Mat VideoFileDataSource::get(size_t index) {
    if (readBackwards) {
        // Decrease video time by number of msec in one frame
        double video_time_ = video_time - frame_msec * index;
        
        if (!(video_time_ > 0)) {
            return cv::Mat();
        }
        
        // and seek to the new time.
        cap.set(cv::CAP_PROP_POS_MSEC, video_time_);
    }
    
    return OpenCVVideoCaptureDataSource::get(index);
}

template<>
FolderDataSource makeDataSource(int argc, char** argv, size_t skip) {
    return FolderDataSource(argc, argv, skip);
}
template<>
CameraDataSource makeDataSource(int argc, char** argv, size_t skip) {
    return CameraDataSource(argc, argv);
}
template<>
VideoFileDataSource makeDataSource(int argc, char** argv, size_t skip) {
    return VideoFileDataSource(argc, argv);
}
