#include <stdlib.h>
#include <stdio.h>
#include "lib_sift.h"
#include "io_png.h"

// Use the OpenCV C API ( http://doc.aldebaran.com/2-0/dev/cpp/examples/vision/opencv.html )
//#include <opencv2/core/core_c.h>

#include <opencv2/opencv.hpp>

int main(int argc, char **argv)
{
    if(argc != 2){
        fprintf(stderr, "usage:\n./exemple2 image\n");
        return -1;
    }

	// Loading image
	size_t w, h;
	float* x = io_png_read_f32_gray(argv[1], &w, &h);
	for(int i=0; i < w*h; i++)
	  x[i] /=256.;

	// compute sift keypoints
	int n;
	//struct sift_keypoint_std *k = sift_compute_features(x, w, h, &n);

	// Make OpenCV matrix with no copy ( https://stackoverflow.com/questions/44453088/how-to-convert-c-array-to-opencv-mat )
	cv::Mat mat(h, w, CV_32F, x);
	imshow("test2", mat);
	cv::waitKey(0);
	
	// write to standard output
	//sift_write_to_file("/dev/stdout", k, n);

	// cleanup
	//free(k);
	free(x);
	return 0;
}
