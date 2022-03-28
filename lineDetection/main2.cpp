// Based on https://docs.opencv.org/3.4/dd/d1a/group__imgproc__feature.html#ga47849c3be0d0406ad3ca45db65a25d2d

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

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace std;
int main(int argc, char** argv)
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
  while(++it != files.end()){
    Mat src, dst, color_dst;
    if( !(src=imread(*it, 0)).data)
        return -1;
    Canny( src, dst, 50, 200, 3 );
    cvtColor( dst, color_dst, COLOR_GRAY2BGR );
    vector<Vec4i> lines;
    HoughLinesP( dst, lines, 1, CV_PI/180, 80, 30, 10 );
    for( size_t i = 0; i < lines.size(); i++ )
    {
        line( color_dst, Point(lines[i][0], lines[i][1]),
        Point( lines[i][2], lines[i][3]), Scalar(0,0,255), 3, 8 );
    }
    std::cout << "Number of lines: " << lines.size() << std::endl;
    namedWindow( "Source", 1 );
    imshow( "Source", src );
    namedWindow( "Detected Lines", 1 );
    imshow( "Detected Lines", color_dst );
    waitKey(0);
  }
  return 0;
}
