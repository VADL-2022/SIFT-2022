//
//  utils.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#pragma once
#include <string>
#include "Config.hpp"

// https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv
// Usage: mat_type2str(cvMat.type())
std::string mat_type2str(int type);

// Throws if fails
std::string openFileWithUniqueName(std::string name, std::string extension);

#if __cplusplus >= 201703L // C++17 and later 
#include <string_view>

bool endsWith(std::string_view str, std::string_view suffix);

bool startsWith(std::string_view str, std::string_view prefix);
#endif // C++17

#if __cplusplus < 201703L // pre C++17
#include <string>

bool endsWith(const std::string& str, const std::string& suffix);

bool startsWith(const std::string& str, const std::string& prefix);

bool endsWith(const std::string& str, const char* suffix, unsigned suffixLen);

bool endsWith(const std::string& str, const char* suffix);

bool startsWith(const std::string& str, const char* prefix, unsigned prefixLen);

bool startsWith(const std::string& str, const char* prefix);
#endif
// //

#ifdef USE_COMMAND_LINE_ARGS
void getScreenResolution(int &width, int &height);
#endif

// For errors that a sigint could handle. Program execution must be able to continue normally after a call to this function, until a thread can teardown.
void recoverableError(const char* msg);

// https://www.cplusplus.com/reference/ctime/strftime/
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#include <sys/stat.h> // https://pubs.opengroup.org/onlinepubs/009695299/functions/mkdir.html
extern std::string cachedDataOutputFolderPath;
const std::string& getDataOutputFolder();
void saveMatrixGivenStr(const cv::Mat& M, std::string& name/*image name output*/,
                        const cv::Ptr<cv::Formatted>& str /*matrix contents input*/);
void matrixToString(const cv::Mat& M,
                    cv::Ptr<cv::Formatted>& str /*matrix contents output*/);
void saveMatrix(const cv::Mat& M, std::string& name/*image name output*/,
                cv::Ptr<cv::Formatted>& str /*matrix contents output*/);
