#include <stdlib.h>
#include <stdio.h>
#include <lib_sift.h>
#include <io_png.h>

// Use the OpenCV C API ( http://doc.aldebaran.com/2-0/dev/cpp/examples/vision/opencv.html )
//#include <opencv2/core/core_c.h>

#include <opencv2/opencv.hpp>

#include "my_sift_additions.h"

#include <filesystem>
namespace fs = std::filesystem;
#include <set>

// Based on https://stackoverflow.com/questions/31658132/c-opencv-not-drawing-circles-on-mat-image and https://stackoverflow.com/questions/19400376/how-to-draw-circles-with-random-colors-in-opencv/19401384
cv::RNG rng(12345); // Random number generator
cv::Scalar nextRNGColor() {
	//return cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255)); // Most of these are rendered as close to white for some reason..
	//return cv::Scalar(0,0,255); // BGR color value (not RGB)
	return cv::Scalar(rng.uniform(0, 255) / rng.uniform(1, 255),
			  rng.uniform(0, 255) / rng.uniform(1, 255),
			  rng.uniform(0, 255) / rng.uniform(1, 255));
}
void drawCircle(cv::Mat& img, cv::Point cp, int radius)
{
    //cv::Scalar black( 0, 0, 0 );
    cv::Scalar color = nextRNGColor();
    
    cv::circle( img, cp, radius, color );
}
void drawRect(cv::Mat& img, cv::Point center, cv::Size2f size, float orientation_degrees) {
	cv::RotatedRect rRect = cv::RotatedRect(center, size, orientation_degrees);
	cv::Point2f vertices[4];
	rRect.points(vertices);
	cv::Scalar color = nextRNGColor();
	//printf("%f %f %f\n", color[0], color[1], color[2]);
	for (int i = 0; i < 4; i++)
		cv::line(img, vertices[i], vertices[(i+1)%4], color, 2);
	
	// To draw a rectangle bounding this one:
	//Rect brect = rRect.boundingRect();
	//cv::rectangle(img, brect, nextRNGColor(), 2);
}
void drawSquare(cv::Mat& img, cv::Point center, int size, float orientation_degrees) {
	drawRect(img, center, cv::Size2f(size, size), orientation_degrees);
}

int main(int argc, char **argv)
{
	// For each output image, loop through it
	std::string path = "outFrames";
	
	// https://stackoverflow.com/questions/62409409/how-to-make-stdfilesystemdirectory-iterator-to-list-filenames-in-order
	//--- filenames are unique so we can use a set
	std::set<fs::path> sorted_by_name;
	for (const auto & entry : fs::directory_iterator(path))
		sorted_by_name.insert(entry.path());
	//--- print the files sorted by filename
	for (auto &path : sorted_by_name) {
		std::cout << path << std::endl;

		// Loading image
		size_t w, h;
		float* x = io_png_read_f32_gray(path.c_str(), &w, &h);
		for(int i=0; i < w*h; i++)
			x[i] /=256.;

		// compute sift keypoints
		int n; // Number of keypoints
		struct sift_keypoint_std *k = my_sift_compute_features(x, w, h, &n);

		// Make OpenCV matrix with no copy ( https://stackoverflow.com/questions/44453088/how-to-convert-c-array-to-opencv-mat )
		cv::Mat mat(h, w, CV_32F, x); // Is black and white

		// Make the black and white OpenCV matrix into color but still black and white (we do this so we can draw colored rectangles on it later)
		cv::Mat backtorgb;
		cv::cvtColor(mat, backtorgb, cv::COLOR_GRAY2RGB); // https://stackoverflow.com/questions/21596281/how-does-one-convert-a-grayscale-image-to-rgb-in-opencv-python

		// Draw keypoints on `mat`
		for(int i=0; i<n; i++){
			drawSquare(backtorgb, cv::Point(k[i].x, k[i].y), k[i].scale, k[i].orientation);
			//break;
			// fprintf(f, "%f %f %f %f ", k[i].x, k[i].y, k[i].scale, k[i].orientation);
			// for(int j=0; j<128; j++){
			// 	fprintf(f, "%u ", k[i].descriptor[j]);
			// }
			// fprintf(f, "\n");
		}
	
		imshow("test2", backtorgb);
		cv::waitKey(0);
	
		// write to standard output
		//sift_write_to_file("/dev/stdout", k, n);

		// cleanup
		//free(k);
		free(x);

	}
	
	return 0;
}
