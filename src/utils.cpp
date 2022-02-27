//
//  utils.cpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "utils.hpp"

#include "opencv_common.hpp"
#include <fstream>
#include <signal.h>
#include "common.hpp"

std::string mat_type2str(int type) {
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT); // Number of channels

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C"; // C = number of channels (for example, RGB has 3 channels because it has three values per pixel, RGBA has 4 channels, etc.)
  r += (chans+'0'); // Number of channels

  return r;
}

// Throws if fails
std::string openFileWithUniqueName(std::string name, std::string extension) {
  // Based on https://stackoverflow.com/questions/13108973/creating-file-names-automatically-c
  
  std::ofstream ofile;

  std::string fname;
  for(unsigned int n = 0; ; ++ n)
    {
      fname = name + std::to_string(n) + extension;

      std::ifstream ifile;
      ifile.open(fname.c_str());

      if(ifile.is_open())
    {
    }
      else
    {
      ofile.open(fname.c_str());
      break;
    }

      ifile.close();
    }

  if(!ofile.is_open())
    {
      //return "";
      { out_guard();
          std::cout << fname << " could not be opened" << std::endl; }
      throw "";
    }

  return fname;
}

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c //
#if __cplusplus >= 201703L // C++17 and later
#include <string_view>

bool endsWith(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool startsWith(std::string_view str, std::string_view prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}
#endif // C++17

#if __cplusplus < 201703L // pre C++17
#include <string>

bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

bool endsWith(const std::string& str, const char* suffix, unsigned suffixLen)
{
    return str.size() >= suffixLen && 0 == str.compare(str.size()-suffixLen, suffixLen, suffix, suffixLen);
}

bool endsWith(const std::string& str, const char* suffix)
{
    return endsWith(str, suffix, std::string::traits_type::length(suffix));
}

bool startsWith(const std::string& str, const char* prefix, unsigned prefixLen)
{
    return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
}

bool startsWith(const std::string& str, const char* prefix)
{
    return startsWith(str, prefix, std::string::traits_type::length(prefix));
}
#endif
// //


// Get desktop size //
// Based on https://stackoverflow.com/questions/11041607/getting-screen-size-on-opencv , https://stackoverflow.com/questions/4921817/get-screen-resolution-programmatically-in-os-x

#ifdef USE_COMMAND_LINE_ARGS
#if WIN32
  #include <windows.h>
#elif defined(__APPLE__)
#include <CoreGraphics/CGDisplayConfiguration.h>
//#include <CoreGraphics/CoreGraphics.h>
#else
  #include <X11/Xlib.h>
#endif

//...

void getScreenResolution(int &width, int &height) {
#if WIN32
    width  = (int) GetSystemMetrics(SM_CXSCREEN);
    height = (int) GetSystemMetrics(SM_CYSCREEN);
#elif defined(__APPLE__)
    auto mainDisplayId = CGMainDisplayID();
    width = CGDisplayPixelsWide(mainDisplayId);
    height = CGDisplayPixelsHigh(mainDisplayId);
#else
    Display* disp = XOpenDisplay(NULL);
    Screen*  scrn = DefaultScreenOfDisplay(disp);
    width  = scrn->width;
    height = scrn->height;
#endif
}
/* Usage:
int main() {
    int width, height;
    getScreenResolution(width, height);
    printf("Screen resolution: %dx%d\n", width, height);
}
*/
#endif
// //

void recoverableError(const char* msg) {
//        throw msg; // Problem is that this generates segfault if no exception handler exists. Instead, we generate a sigint to stop the program:
    
    // https://www.tutorialspoint.com/c_standard_library/c_function_raise.htm
    printf("%s, going to raise a signal\n", msg);
    int ret = raise(SIGINT);
    
    if( ret !=0 ) {
       printf("Error: unable to raise SIGINT signal.\n");
       throw msg;
    }
}
