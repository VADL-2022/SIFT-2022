//
//  DataOutput.cpp
//  SIFT
//
//  Created by VADL on 11/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "DataOutput.hpp"

#include "opencv2/highgui.hpp"
#include <fstream>
#include "utils.hpp"

void PreviewWindowDataOutput::init(size_t width, size_t height) {
    // Initialize canvas if needed
    if (canvas.data == nullptr) {
        puts("Init canvas");
        canvas = cv::Mat(height, width, CV_32FC4);
    }
}

void PreviewWindowDataOutput::showCanvas(std::string name) {
    showCanvas(name, canvas);
}
void PreviewWindowDataOutput::showCanvas(std::string name, cv::Mat& canvas) {
    t.reset();
    imshow(name, canvas);
    t.logElapsed("show canvas window");
}

int PreviewWindowDataOutput::waitKey(int delay) {
    return cv::waitKey(delay);
}

FileDataOutput::FileDataOutput(std::string filenameNoExt_, double fps_, cv::Size sizeFrame_) :
fps(fps_), sizeFrame(sizeFrame_), filenameNoExt(filenameNoExt_) {}

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
      throw "";
    }

  return fname;
}

void FileDataOutput::run(cv::Mat frame) {
    bool first = !writer.isOpened();
    if (first) {
        int codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

        bool isColor = (frame.type() == CV_8UC3);
        std::cout << "Output type: " << type2str(frame.type()) << "\nisColor: " << isColor << std::endl;
        std::string filename = openFileWithUniqueName(filenameNoExt, ".mp4");
        writer.open(filename, codec, fps, sizeFrame, isColor);
    }
    
    writer.write(frame);
}

void FileDataOutput::showCanvas(std::string name) {
    showCanvas(name, canvas);
}
void FileDataOutput::showCanvas(std::string name, cv::Mat& canvas) {
    t.reset();
    run(canvas);
    t.logElapsed("save frame to FileDataOutput");
}

int FileDataOutput::waitKey(int delay) {
    // Advance to next image after user presses enter
    do {
        std::cout << "Press enter to advance to the next frame and get keypoints, g to load or make cached keypoints file, s to apply previous transformations, or q + enter to quit: " << std::flush;
        std::string n;
        std::getline(std::cin, n);
        if (n.empty()) {
            return 'd'; // Simply advance to next image
        }
        else if (n.length() > 1) {
            std::cout << "Invalid option. Try again." << std::endl;
            continue;
        }
        else {
            return n[0]; // The command given
        }
        break;
    } while (true);
}
