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

FolderDataSource::FolderDataSource(std::string folderPath, size_t skip_ = 0): currentIndex(skip_) {
    init(folderPath);
}

// Parses skip from a command-line argument
size_t getSkip(char* arg) {
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
    printf("Using skip %zu\n", skip_); // Note: if you give a negative number: "If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type." ( https://en.cppreference.com/w/c/string/byte/strtoimax )
    return skip_;
}
FolderDataSource::FolderDataSource(int argc, char** argv, size_t skip): currentIndex(skip) {
    // Set the default folder path
    std::string folderPath = "testFrames2_cropped"; //"testFrames1";
    //std::string folderPath = "outFrames";
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (i+1 < argc) {
            if (strcmp(argv[i], "--folder") == 0) {
                folderPath = argv[i+1];
                i++;
            }
            else if (strcmp(argv[i], "--skip") == 0) {
                skip = getSkip(argv[i+1]);
                i++;
            }
            else {
                goto else_;
            }
        }
        else {
else_:
            if (argc > 1) {
                skip = getSkip(argv[1]);
            }
        }
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
cv::Mat CameraDataSource::siftImageForMat(size_t index) {
    cv::Mat grey = cache.at(index), mat;
    t.reset();
    std::cout << type2str(grey.type()) << std::endl;
    cv::cvtColor(grey, mat, cv::COLOR_RGB2GRAY);
    mat.convertTo(mat, CV_32F);
    std::cout << type2str(mat.type()) << std::endl;
    t.logElapsed("convert image to greyscale and float");
    return mat;
}
cv::Mat CameraDataSource::colorImageForMat(size_t index) {
    return cache.at(index);
}

CameraDataSource::CameraDataSource() {
    init();
}

CameraDataSource::CameraDataSource(int argc, char** argv) {
    init();
}

void CameraDataSource::init() {
    double fps;
    //cv::Size sizeFrame(640,480);
    cv::Size sizeFrame(1920,1080);
    
    // Instanciate the capture
    cap.open(0);

    if (!cap.isOpened()) { // check if we succeeded
      std::cout << "Unable to start capture" << std::endl;
      throw 0;
    }

    // https://learnopencv.com/how-to-find-frame-rate-or-frames-per-second-fps-in-opencv-python-cpp/
    // "For OpenCV 3, you can also use the following"
    fps = cap.get(cv::CAP_PROP_FPS);

    cap.set(cv::CAP_PROP_FRAME_WIDTH, sizeFrame.width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, sizeFrame.height);
    double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Frames per second using video.get(CAP_PROP_FPS) : " << fps << "\nWidth: " << width << "\nHeight: " << height << std::endl;

    
    //cv::Mat frame_;
    // int i = 0;
    // while ((i++ < 100) && !cap.read(frame_)) {
    // }; // warm up camera by skiping unread frames
}

bool CameraDataSource::hasNext() {
    return cap.isOpened(); // TODO: make this return whether we "sync"ed with the camera or not since the last grab. Use https://man7.org/linux/man-pages/man2/select.2.html and /Volumes/MyTestVolume/MiscRepos/opencv/modules/videoio/src/cap_v4l.cpp:996 to change timeout of select() system call to instant so it doesn't hang around waiting for data. Low level stuff explained: https://web.archive.org/web/20140401235613/http://pages.cpsc.ucalgary.ca/~sayles/VFL_HowTo/
    // https://www.tutorialspoint.com/unix_system_calls/_newselect.htm : "timeout is an upper bound on the amount of time elapsed before select() returns. It may be zero, causing select() to return immediately. (This is useful for polling.) If timeout is NULL (no timeout), select() can block indefinitely."
}

cv::Mat CameraDataSource::next() {
    return get(currentIndex++);
}

cv::Mat CameraDataSource::get(size_t index) {
    size_t i = index;
    
    auto it = cache.find(i);
    if (it != cache.end()) {
        return it->second;
    }
    
    if (i < currentIndex) {
        throw "Index given has no cached image";
    }
    else if (i == currentIndex) {
        currentIndex++;
    }
    else if (i > currentIndex) { // Index given is too large
        return cv::Mat();
    }
    
    cv::Mat mat;
    if (!cap.read(mat)) {
        throw "Error reading from camera";
    }
    
    // Cache it
    cache.emplace(i, mat);
    
    return mat;
}

std::string CameraDataSource::nameForIndex(size_t index) {
    return "Image " + std::to_string(index);
}

template<>
FolderDataSource makeDataSource(int argc, char** argv, size_t skip) {
    return FolderDataSource(argc, argv, skip);
}
template<>
CameraDataSource makeDataSource(int argc, char** argv, size_t skip) {
    return CameraDataSource(argc, argv);
}
