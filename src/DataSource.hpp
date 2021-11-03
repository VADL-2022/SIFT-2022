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

// Note: cv::Mat is reference counted automatically

template <typename Wrapped>
struct DataSourceBase {
    bool hasNext();
    cv::Mat next();
    cv::Mat get(size_t index);
    std::string nameForIndex(size_t index);
    float* dataForMat(size_t index);
};

struct FolderDataSource : public DataSourceBase<FolderDataSource>
{
    FolderDataSource(std::string folderPath, size_t skip);
    FolderDataSource(int argc, char** argv, size_t skip);
    ~FolderDataSource();
    
    bool hasNext();
    cv::Mat next();
    
    cv::Mat get(size_t index);
    
    std::string nameForIndex(size_t index);
    float* dataForMat(size_t index);
    
    size_t currentIndex; // Index to save into next
    std::unordered_map<size_t, cv::Mat> cache;
private:
    std::vector<std::string> files;
    
    void init(std::string folderPath);
};

struct CameraDataSource : public DataSourceBase<CameraDataSource>
{
    CameraDataSource();
    CameraDataSource(int argc, char** argv);
    
    bool hasNext();
    cv::Mat next();
    
    cv::Mat get(size_t index);
    
    std::string nameForIndex(size_t index);
    float* dataForMat(size_t index);
    
    cv::VideoCapture cap;
    size_t currentIndex; // Index to save into next
    std::unordered_map<size_t, cv::Mat> cache;
private:
    void init();
};

// Parital template specialization. But maybe use this: https://stackoverflow.com/questions/31500426/why-does-enable-if-t-in-template-arguments-complains-about-redefinitions
template<class T>
extern T makeDataSource(int argc, char** argv, size_t skip) { throw 0; }
template<>
FolderDataSource makeDataSource(int argc, char** argv, size_t skip);
template<>
CameraDataSource makeDataSource(int argc, char** argv, size_t skip);

#endif /* DataSource_hpp */
