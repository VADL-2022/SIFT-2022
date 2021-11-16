//
//  DataSource.hpp
//  SIFT
//
//  Created by VADL on 11/2/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef DataSource_hpp
#define DataSource_hpp

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <utility>
#include <unordered_map>
#include "Config.hpp"

#ifdef USE_COMMAND_LINE_ARGS
#define MaybeVirtual virtual
#define MaybePureVirtual = 0
#else
#define MaybeVirtual
#define MaybePureVirtual
#endif

// Note: cv::Mat is reference counted automatically

struct DataSourceBase {
    #ifdef USE_COMMAND_LINE_ARGS
    virtual ~DataSourceBase() {}
    #endif
    
    MaybeVirtual bool hasNext() MaybePureVirtual;
    MaybeVirtual cv::Mat next() MaybePureVirtual;
    MaybeVirtual cv::Mat get(size_t index) MaybePureVirtual;
    MaybeVirtual std::string nameForIndex(size_t index) MaybePureVirtual;
    MaybeVirtual float* dataForMat(size_t index) MaybePureVirtual;
    
    MaybeVirtual cv::Mat siftImageForMat(size_t index) MaybePureVirtual;
    MaybeVirtual cv::Mat colorImageForMat(size_t index) MaybePureVirtual;
    
    MaybeVirtual bool wantsCustomFPS() const MaybePureVirtual;
    MaybeVirtual double fps() const MaybePureVirtual;
    MaybeVirtual double timeMilliseconds() const MaybePureVirtual;
    
    size_t currentIndex; // Index to save into next
};

struct FolderDataSource : public DataSourceBase
{
    FolderDataSource(std::string folderPath, size_t skip);
    FolderDataSource(int argc, char** argv, size_t skip);
    ~FolderDataSource();
    
    bool hasNext();
    cv::Mat next();
    
    cv::Mat get(size_t index);
    
    std::string nameForIndex(size_t index);
    float* dataForMat(size_t index) { return (float*)get(index).data; }
    cv::Mat siftImageForMat(size_t index);
    cv::Mat colorImageForMat(size_t index);
    
    bool wantsCustomFPS() const { return false; }
    double fps() const { return DBL_MAX; }
    double timeMilliseconds() const { return 0; }
    
    size_t currentIndex; // Index to save into next
    std::unordered_map<size_t, cv::Mat> cache;
private:
    std::vector<std::string> files;
    cv::Size sizeFrame;
    
    void init(std::string folderPath);
};

struct OpenCVVideoCaptureDataSource : public DataSourceBase
{
    OpenCVVideoCaptureDataSource();
    
    bool hasNext() { return cap.isOpened(); }
    cv::Mat next();
    
    cv::Mat get(size_t index);
    
    std::string nameForIndex(size_t index);
    float* dataForMat(size_t index) { return (float*)get(index).data; }
    cv::Mat siftImageForMat(size_t index);
    cv::Mat colorImageForMat(size_t index);
    
    bool wantsCustomFPS() const { return wantedFPS.has_value(); }
    double fps() const { return wantsCustomFPS() ? wantedFPS.value() : cap.get(cv::CAP_PROP_FPS); }
    double timeMilliseconds() const { return cap.get(cv::CAP_PROP_POS_MSEC); }

protected:
    std::optional<double> wantedFPS;
    
    static const double default_fps;
    
    cv::VideoCapture cap;
    std::unordered_map<size_t, cv::Mat> cache;
};

struct CameraDataSource : public OpenCVVideoCaptureDataSource
{
    CameraDataSource();
    CameraDataSource(int argc, char** argv);
    
    bool hasNext();
    
    using OpenCVVideoCaptureDataSource::get;
    
protected:
    static const cv::Size default_sizeFrame;
    void init(double fps, cv::Size sizeFrame);
};

struct VideoFileDataSource : public OpenCVVideoCaptureDataSource
{
    VideoFileDataSource();
    VideoFileDataSource(int argc, char** argv);
    
    cv::Mat get(size_t index);
    
private:
    double frame_rate;
    double frame_msec;
    double video_time;
protected:
    static const std::string default_filePath;
    void init(std::string filePath);
};


// Parital template specialization. But maybe use this: https://stackoverflow.com/questions/31500426/why-does-enable-if-t-in-template-arguments-complains-about-redefinitions
template<class T>
extern T makeDataSource(int argc, char** argv, size_t skip) { throw 0; }
template<>
FolderDataSource makeDataSource(int argc, char** argv, size_t skip);
template<>
CameraDataSource makeDataSource(int argc, char** argv, size_t skip);
template<>
VideoFileDataSource makeDataSource(int argc, char** argv, size_t skip);

#endif /* DataSource_hpp */
