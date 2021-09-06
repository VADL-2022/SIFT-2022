#include <stdlib.h>
#include <stdio.h>
#include <lib_sift.h>
#include <lib_keypoint.h>
#include <lib_matching.h>
#include <io_png.h>

// Use the OpenCV C API ( http://doc.aldebaran.com/2-0/dev/cpp/examples/vision/opencv.html )
//#include <opencv2/core/core_c.h>

#include <opencv2/opencv.hpp>

#include "my_sift_additions.h"

#include <filesystem>
namespace fs = std::filesystem;
#include "strnatcmp.hpp"

// Based on https://stackoverflow.com/questions/31658132/c-opencv-not-drawing-circles-on-mat-image and https://stackoverflow.com/questions/19400376/how-to-draw-circles-with-random-colors-in-opencv/19401384
#define RNG_SEED 12345
cv::RNG rng(RNG_SEED); // Random number generator
cv::Scalar lastColor;
void resetRNG() {
	rng = cv::RNG(RNG_SEED);
}
cv::Scalar nextRNGColor() {
	//return cv::Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255)); // Most of these are rendered as close to white for some reason..
	//return cv::Scalar(0,0,255); // BGR color value (not RGB)
	lastColor = cv::Scalar(rng.uniform(0, 255) / rng.uniform(1, 255),
			       rng.uniform(0, 255) / rng.uniform(1, 255),
			       rng.uniform(0, 255) / rng.uniform(1, 255));
	return lastColor;
}
void drawCircle(cv::Mat& img, cv::Point cp, int radius)
{
    //cv::Scalar black( 0, 0, 0 );
    cv::Scalar color = nextRNGColor();
    
    cv::circle( img, cp, radius, color );
}
void drawRect(cv::Mat& img, cv::Point center, cv::Size2f size, float orientation_degrees, int thickness = 2) {
	cv::RotatedRect rRect = cv::RotatedRect(center, size, orientation_degrees);
	cv::Point2f vertices[4];
	rRect.points(vertices);
	cv::Scalar color = nextRNGColor();
	//printf("%f %f %f\n", color[0], color[1], color[2]);
	for (int i = 0; i < 4; i++)
		cv::line(img, vertices[i], vertices[(i+1)%4], color, thickness);
	
	// To draw a rectangle bounding this one:
	//Rect brect = rRect.boundingRect();
	//cv::rectangle(img, brect, nextRNGColor(), 2);
}
void drawSquare(cv::Mat& img, cv::Point center, int size, float orientation_degrees, int thickness = 2) {
	drawRect(img, center, cv::Size2f(size, size), orientation_degrees, thickness);
}

int main(int argc, char **argv)
{
	// For each output image, loop through it
	std::string path = "outFrames";
	
	// https://stackoverflow.com/questions/62409409/how-to-make-stdfilesystemdirectory-iterator-to-list-filenames-in-order
	//--- filenames are unique so we can use a set
	std::vector<std::string> files;
	for (const auto & entry : fs::directory_iterator(path))
		files.push_back(entry.path().string());

	// https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
	std::sort(files.begin(),files.end(),compareNat);
	
	//--- print the files sorted by filename
	std::vector<struct sift_keypoints*> computedKeypoints;
	size_t skip = 100;//60;//100;//38;//0;
	for (size_t i = skip; i < files.size(); i++) {
		auto& path = files[i];
		std::cout << path << std::endl;

		// Loading image
		size_t w, h;
		float* x = io_png_read_f32_gray(path.c_str(), &w, &h);
		for(int i=0; i < w*h; i++)
			x[i] /=256.; // TODO: why do we do this?

		// compute sift keypoints
		int n; // Number of keypoints
		struct sift_keypoints* keypoints;
		struct sift_keypoint_std *k = my_sift_compute_features(x, w, h, &n, &keypoints);

		// Make OpenCV matrix with no copy ( https://stackoverflow.com/questions/44453088/how-to-convert-c-array-to-opencv-mat )
		cv::Mat mat(h, w, CV_32F, x); // Is black and white

		// Make the black and white OpenCV matrix into color but still black and white (we do this so we can draw colored rectangles on it later)
		cv::Mat backtorgb;
		cv::cvtColor(mat, backtorgb, cv::COLOR_GRAY2RGB); // https://stackoverflow.com/questions/21596281/how-does-one-convert-a-grayscale-image-to-rgb-in-opencv-python

		// Draw keypoints on `mat`
		for(int i=0; i<n; i++){
			drawSquare(backtorgb, cv::Point(k[i].x, k[i].y), k[i].scale, k[i].orientation, 1);
			//break;
			// fprintf(f, "%f %f %f %f ", k[i].x, k[i].y, k[i].scale, k[i].orientation);
			// for(int j=0; j<128; j++){
			// 	fprintf(f, "%u ", k[i].descriptor[j]);
			// }
			// fprintf(f, "\n");
		}

		// Compare keypoints if we had some previously
		if (computedKeypoints.size() > 0) {
			struct sift_keypoints* keypointsPrev = computedKeypoints.back();
			struct sift_keypoints* out_k1 = sift_malloc_keypoints();
			struct sift_keypoints* out_k2A = sift_malloc_keypoints();
			struct sift_keypoints* out_k2B = sift_malloc_keypoints();
			
			// Setting default parameters
			int n_hist = 4;
			int n_ori = 8;
			int n_bins = 36;
			int meth_flag = 1;
			float thresh = 0.6;
			int verb_flag = 0;
			char label[256];
    
			// Matching
			matching(keypointsPrev, keypoints, out_k1, out_k2A, out_k2B, thresh, meth_flag);

			// Draw matches
			struct sift_keypoints* k1 = out_k1;
			printf("Number of matching keypoints: %d\n", k1->size);
			if (k1->size > 0){

				int n_hist = k1->list[0]->n_hist;
				int n_ori = k1->list[0]->n_ori;
				int dim = n_hist*n_hist*n_ori;
				int n_bins  = k1->list[0]->n_bins;
				int n = k1->size;
				for(int i = 0; i < n; i++){
					// fprintf_one_keypoint(f, k1->list[i], dim, n_bins, 2);
					// fprintf_one_keypoint(f, k2A->list[i], dim, n_bins, 2);
					// fprintf_one_keypoint(f, k2B->list[i], dim, n_bins, 2);
					// fprintf(f, "\n");

					drawSquare(backtorgb, cv::Point(out_k2A->list[i]->x, out_k2A->list[i]->y), out_k2A->list[i]->sigma /* need to choose something better here */, out_k2A->list[i]->theta, 2);
					cv::line(backtorgb, cv::Point(out_k1->list[i]->x, out_k1->list[i]->y), cv::Point(out_k2A->list[i]->x, out_k2A->list[i]->y), lastColor, 1);
				}
			}

			// Cleanup //
			sift_free_keypoints(out_k1);
			sift_free_keypoints(out_k2A);
			sift_free_keypoints(out_k2B);
			
			sift_free_keypoints(keypointsPrev);
			computedKeypoints.pop_back();
			// //
		}
		
		imshow(path, backtorgb);
		cv::waitKey(0);
	
		// write to standard output
		//sift_write_to_file("/dev/stdout", k, n);

		// cleanup
		free(k);
		free(x);

		// Save keypoints
		computedKeypoints.push_back(keypoints);
		
		// Reset RNG so some colors coincide
		resetRNG();
	}

	// Cleanup
	for (struct sift_keypoints* keypoints : computedKeypoints) {
		sift_free_keypoints(keypoints);
	}
	
	return 0;
}
