// Based on https://docs.opencv.org/3.4/d4/dee/tutorial_optical_flow.html
// Usage: `bash compile.sh && ./myTest ../Data/fullscale1/Derived/SIFT/ExtractedFrames .png`

#include "../src/DataSource.hpp"
#include <filesystem>
namespace fs = std::filesystem;
#include "strnatcmp.hpp"
#include "utils.hpp"
#include "common.hpp"

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
using namespace cv;
using namespace std;
int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cout << "This command runs on all files matching a certain extension in a directory given. It does this traversal in a \"natural\"/\"human-readable\" way with respect to numbers in that it goes through numbers contained in filenames in increasing order instead of lexicographically, while also going through the filenames as alphabetically sorted.\n\tUsage: " << argv[0] << " <folder path> [optional extensions to match, else .mp4 is matched] [optional number of images to skip at the start]\n\tExample: " << argv[0] << " folder . ''      (to match all extensions)" << std::endl;
    exit(1);
  }
  
  std::vector<std::string> files;
  std::string folderPath = argv[1];
  std::string extMatch = ".mp4";
  if (argc >= 3) {
    extMatch = argv[2];
  }
  int skip = 0;
  if (argc >= 4) {
    skip = std::stoi(argv[3]);
  }
  for (const auto &entry : fs::directory_iterator(folderPath)) {
    // Ignore files we treat specially:
    // if (entry.is_directory() || endsWith(entry.path().string(),
    // ".keypoints.txt") || endsWith(entry.path().string(), ".matches.txt") ||
    // entry.path().filename().string() == ".DS_Store") {
    //     continue;
    // }
    if (!endsWith(entry.path().string(), extMatch)) {
      continue;
    }
    files.push_back(entry.path().string());
  }

  // https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
  std::sort(files.begin(), files.end(), compareNat);

  auto it = files.begin() + skip;
  
  if (true) {  
    // Create some random colors
    vector<Scalar> colors;
    RNG rng;
    for(int i = 0; i < 100; i++)
    {
        int r = rng.uniform(0, 256);
        int g = rng.uniform(0, 256);
        int b = rng.uniform(0, 256);
        colors.push_back(Scalar(r,g,b));
    }
    Mat old_frame, old_gray;
    vector<Point2f> p0, p1;
    // Take first frame and find corners in it
    if (it == files.end()) {
      return 0;
    }
    old_frame = imread(*it);
    cvtColor(old_frame, old_gray, COLOR_BGR2GRAY);
    goodFeaturesToTrack(old_gray, p0, 100, 0.3, 7, Mat(), 7, false, 0.04);
    // Create a mask image for drawing purposes
    Mat mask = Mat::zeros(old_frame.size(), old_frame.type());
    while(++it != files.end()){
        Mat frame, frame_gray;
        frame = imread(*it);
        if (frame.empty())
            break;
        cvtColor(frame, frame_gray, COLOR_BGR2GRAY);

	// DENSE FLOW //

	cv::Mat& prvs = old_gray, &next = frame_gray;
	
        Mat flow(prvs.size(), CV_32FC2);
        calcOpticalFlowFarneback(prvs, next, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
        // visualization
        Mat flow_parts[2];
        split(flow, flow_parts);
        Mat magnitude, angle, magn_norm;
        cartToPolar(flow_parts[0], flow_parts[1], magnitude, angle, true);
        normalize(magnitude, magn_norm, 0.0f, 1.0f, NORM_MINMAX);
        angle *= ((1.f / 360.f) * (180.f / 255.f));
        //build hsv image
        Mat _hsv[3], hsv, hsv8, bgr;
        _hsv[0] = angle;
        _hsv[1] = Mat::ones(angle.size(), CV_32F);
        _hsv[2] = magn_norm;
        merge(_hsv, 3, hsv);
        hsv.convertTo(hsv8, CV_8U, 255.0);
        cvtColor(hsv8, bgr, COLOR_HSV2BGR);

	// SHOW DENSE FLOW //
	
        imshow("frame2", bgr);
	
	// SPARSE FLOW //
	
        // calculate optical flow
        vector<uchar> status;
        vector<float> err;
        TermCriteria criteria = TermCriteria((TermCriteria::COUNT) + (TermCriteria::EPS), 10, 0.03);
	std::cout << "p0.size(): " << p0.size() << std::endl;
	std::cout << "p1.size(): " << p1.size() << std::endl;
        if (p0.size() == 0) {
	  std::cout << "p0.size() is 0, recomputing tracking features" << std::endl;
	  goodFeaturesToTrack(frame_gray, p0, 100, 0.3, 7, Mat(), 7, false, 0.04);
	  p1.clear();
	  mask = Mat::zeros(frame.size(), frame.type());
	  
          //std::cout << "p0.size() is 0, so calcOpticalFlowPyrLK would crash. Here goes:" << std::endl;
          //return 1;
        }
        calcOpticalFlowPyrLK(old_gray, frame_gray, p0, p1, status, err, Size(15,15), 2, criteria);
        vector<Point2f> good_new;
        for(uint i = 0; i < p0.size(); i++)
        {
            // Select good points
            if(status[i] == 1) {
                good_new.push_back(p1[i]);
                // draw the tracks
                line(mask,p1[i], p0[i], colors[i], 2);
                circle(frame, p1[i], 5, colors[i], -1);
            }
        }
        Mat img;
        add(frame, mask, img);

	// SHOW SPARSE FLOW //
	
        imshow("Frame", img);

	// WAIT FOR KEY //
	
        int keyboard = waitKey(0);//waitKey(30);
        if (keyboard == 'q' || keyboard == 27)
            break;
        // Now update the previous frame and previous points
        old_gray = frame_gray.clone();
        p0 = good_new;
    }
  }
}
