//
//  utils.cpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "utils.hpp"

#include <opencv2/opencv.hpp>
#include <fstream>

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
        std::cout << fname << " could not be opened" << std::endl;
      throw "";
    }

  return fname;
}
