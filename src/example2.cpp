#include <stdlib.h>
#include <stdio.h>
#include <lib_sift.h>
#include <lib_sift_anatomy.h>
#include <lib_keypoint.h>
#include <lib_matching.h>
#include <lib_util.h>
#include <io_png.h>

// Use the OpenCV C API ( http://doc.aldebaran.com/2-0/dev/cpp/examples/vision/opencv.html )
//#include <opencv2/core/core_c.h>

#include <opencv2/opencv.hpp>

#include "my_sift_additions.h"

#include <filesystem>
//namespace fs = std::filesystem;
namespace fs = std::__fs::filesystem;
#include "strnatcmp.hpp"

#include "opencv2/highgui.hpp"
#include <optional>
#include <cinttypes>
#include <iostream>
#include "utils.hpp"

#include "Timer.hpp"
Timer t;

// https://docs.opencv.org/master/da/d6a/tutorial_trackbar.html
const int alpha_slider_max = 2;
int alpha_slider = 0;
double alpha;
double beta;
static void on_trackbar( int, void* )
{
   alpha = (double) alpha_slider/alpha_slider_max ;
}


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

// Wrapper around imshow that reuses the same window each time
void imshow(std::string name, cv::Mat& mat) {
    static std::optional<std::string> prevWindowTitle;
    
    if (prevWindowTitle) {
        // Rename window
        cv::setWindowTitle(*prevWindowTitle, name);
    }
    else {
        prevWindowTitle = name;
        cv::namedWindow(name, cv::WINDOW_AUTOSIZE); // Create Window
        
//        // Set up window
//        char TrackbarName[50];
//        sprintf( TrackbarName, "Alpha x %d", alpha_slider_max );
//        cv::createTrackbar( TrackbarName, name, &alpha_slider, alpha_slider_max, on_trackbar );
//        on_trackbar( alpha_slider, 0 );
    }
    cv::imshow(*prevWindowTitle, mat); // prevWindowTitle (the first title) is always used as an identifier for this window, regardless of the renaming done via setWindowTitle().
}

int main(int argc, char **argv)
{
	// Set the default "skip"
	size_t skip = 0;//120;//60;//100;//38;//0;
	if(argc >= 2){
		// Use given skip (selects the first image to show)
		char* endptr;
		std::uintmax_t skip_ = std::strtoumax(argv[1], &endptr, 0); // https://en.cppreference.com/w/cpp/string/byte/strtoimax
		// Check for overflow
		auto max = std::numeric_limits<size_t>::max();
		if (skip_ > max || ERANGE == errno) {
			std::cout << "Argument (\"" << skip_ << "\") is too large for size_t (max value is " << max << "). Exiting." << std::endl;
			return 1;
		}
		// Check for conversion failures ( https://wiki.sei.cmu.edu/confluence/display/c/ERR34-C.+Detect+errors+when+converting+a+string+to+a+number )
		else if (endptr == argv[1]) {
			std::cout << "Argument (\"" << argv[1] << "\") could not be converted to an unsigned integer. Exiting." << std::endl;
			return 2;
		}
		skip = skip_;
		printf("Using skip %zu\n", skip); // Note: if you give a negative number: "If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type." ( https://en.cppreference.com/w/c/string/byte/strtoimax )
	}
	
	// For each output image, loop through it
	std::string folderPath = "testFrames2_cropped"; //"testFrames1";
	//std::string folderPath = "outFrames";
	
	// https://stackoverflow.com/questions/62409409/how-to-make-stdfilesystemdirectory-iterator-to-list-filenames-in-order
	//--- filenames are unique so we can use a set
	std::vector<std::string> files;
	for (const auto & entry : fs::directory_iterator(folderPath)) {
		// Ignore files we treat specially:
		if (endsWith(entry.path().string(), ".keypoints.txt")) {
			continue;
		}
		files.push_back(entry.path().string());
	}

	// https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
	std::sort(files.begin(),files.end(),compareNat);
	
	//--- print the files sorted by filename
	std::vector<struct sift_keypoints*> computedKeypoints;
	cv::Mat canvas;
	std::vector<cv::Mat> allTransformations;
	cv::Mat firstImage;
    std::unique_ptr<struct sift_keypoints> loadedKeypoints;
	std::unique_ptr<struct sift_keypoint_std> loadedK;
	int loadedKeypointsSize = 0;
	// Set params //
	/** assign parameters **/
	struct sift_parameters* params = sift_assign_default_parameters();
	v3Params(params);
	//v2Params(params);
	// //
	bool checkLoadedK = true;
	for (size_t i = skip; i < files.size(); i++) {
		//for (size_t i = files.size() - 1 - skip; i < files.size() /*underflow of i will end the loop*/; i--) {
		auto& path = files[i];
		std::cout << path << std::endl;

		// Loading image
        t.reset();
		size_t w, h;
		float* x = io_png_read_f32_gray(path.c_str(), &w, &h);
        t.logElapsed("load image");
        t.reset();
		for(int i=0; i < w*h; i++)
			x[i] /=256.; // TODO: why do we do this?
        t.logElapsed("normalize image");

		// Initialize canvas if needed
		if (canvas.data == nullptr) {
			puts("Init canvas");
			canvas = cv::Mat(h, w, CV_32FC4);
		}

		// compute sift keypoints
		int n; // Number of keypoints
		struct sift_keypoints* keypoints;
		struct sift_keypoint_std *k;
		if (loadedKeypoints == nullptr) {
            t.reset();
			k = my_sift_compute_features(params, x, w, h, &n, &keypoints);
            t.logElapsed("compute features");
		}
		else {
			k = loadedK.release(); // "Releases the ownership of the managed object if any." ( https://en.cppreference.com/w/cpp/memory/unique_ptr/release )
            keypoints = loadedKeypoints.release();
			n = loadedKeypointsSize;
		}

		// Make OpenCV matrix with no copy ( https://stackoverflow.com/questions/44453088/how-to-convert-c-array-to-opencv-mat )
		cv::Mat mat(h, w, CV_32F, x); // Is black and white

		// Make the black and white OpenCV matrix into color but still black and white (we do this so we can draw colored rectangles on it later)
		cv::Mat backtorgb;
        t.reset();
		cv::cvtColor(mat, backtorgb, cv::COLOR_GRAY2RGBA); // https://stackoverflow.com/questions/21596281/how-does-one-convert-a-grayscale-image-to-rgb-in-opencv-python
        t.logElapsed("convert image");
		if (i == skip) {
			puts("Init firstImage");
			firstImage = backtorgb;
		}

		// Draw keypoints on `mat`
        t.reset();
		for(int i=0; i<n; i++){
			drawSquare(backtorgb, cv::Point(k[i].x, k[i].y), k[i].scale, k[i].orientation, 1);
			//break;
			// fprintf(f, "%f %f %f %f ", k[i].x, k[i].y, k[i].scale, k[i].orientation);
			// for(int j=0; j<128; j++){
			// 	fprintf(f, "%u ", k[i].descriptor[j]);
			// }
			// fprintf(f, "\n");
		}
        t.logElapsed("draw keypoints");

		// Compare keypoints if we had some previously
		if (computedKeypoints.size() > 0) {
            t.reset();
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
            t.logElapsed("find matches");

			// Draw matches
            t.reset();
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
            t.logElapsed("draw matches");

            t.reset()
			// Find the homography matrix between the previous and current image ( https://docs.opencv.org/4.5.2/d1/de0/tutorial_py_feature_homography.html )
			//const int MIN_MATCH_COUNT = 10
			// https://docs.opencv.org/3.4/d7/dff/tutorial_feature_homography.html
			std::vector<cv::Point2f> obj; // The `obj` is the current image's keypoints
			std::vector<cv::Point2f> scene; // The `scene` is the previous image's keypoints. "Scene" means the area within which we are finding `obj` and this is since we want to find the previous image in the current image since we're falling down and therefore "zooming in" so the current image should be within the previous one.
			obj.reserve(k1->size);
			scene.reserve(k1->size);
			// TODO: reuse memory of k1 instead of copying?
			for (size_t i = 0; i < k1->size; i++) {
				obj.emplace_back(out_k2A->list[i]->x, out_k2A->list[i]->y);
				scene.emplace_back(out_k1->list[i]->x, out_k1->list[i]->y);
			}
			
			// Make a matrix in transformations history
			allTransformations.emplace_back(cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ ));
            t.logElapsed("find homography");
			
            t.reset();
			// Save to canvas
			cv::Mat& img_matches = backtorgb; // The image on which to draw the lines showing corners of the object (current image)
			img_matches.copyTo(canvas);
            t.logElapsed("render to canvas: with prev keypoints");
			
			// Cleanup //
			sift_free_keypoints(out_k1);
			sift_free_keypoints(out_k2A);
			sift_free_keypoints(out_k2B);
			
			sift_free_keypoints(keypointsPrev);
			computedKeypoints.pop_back();
			// //
		}
		else {
            t.reset();
			// Save to canvas
			backtorgb.copyTo(canvas);
            t.logElapsed("render to canvas: no prev keypoints");
		}
		
		//imshow(path, backtorgb);

        t.reset();
		imshow(path, canvas);
        t.logElapsed("show canvas window");
		int keycode = cv::waitKey(0);
		bool exit;
		size_t currentTransformation = allTransformations.size() - 1;
		cv::Mat M = allTransformations.size() > 0 ? allTransformations[currentTransformation] : cv::Mat();
		cv::Mat* H;
		cv::Mat temp;
		cv::Mat& img_object = backtorgb; // The image containing the "object" (current image)
		cv::Mat& img_matches = backtorgb; // The image on which to draw the lines showing corners of the object (current image)
		std::string fname = i + 1 >= files.size() ? "" : files[i+1] + ".keypoints.txt";
        struct sift_keypoints* ptr;
		do {
			int counter = 0;
			exit = true;
			switch (keycode) {
			case 'a':
				// Go to previous image
				i -= 2;
				break;
			case 'f':
				// 'f' for "file" writing
				// Go to next image and save SIFT keypoints
				
				// Serialize to file
                t.reset();
				printf("Saving keypoints to %s\n", fname.c_str());
				my_sift_write_to_file(fname.c_str(), keypoints, params, n); //sift_write_to_file(fname.c_str(), k, n); //<--not used because it doesn't save params
                t.logElapsed("save keypoints");
				break;
			case 'g':
				// 'g' for "get"
				// Go to next image and get its keypoints from a file
                t.reset();
				int numKeypoints;
				if (fname.empty()) {
					printf("No next keypoints file to load from\n");
					break;
				}
				printf("Loading keypoints from next image's keypoints file %s\n", fname.c_str());
                ptr = loadedKeypoints.get(); // Make regular pointer so we can use a double pointer below
				loadedK.reset(my_sift_read_from_file(fname.c_str(), &numKeypoints, &ptr)); // https://en.cppreference.com/w/cpp/memory/unique_ptr/reset
                if (loadedK == nullptr) {
                    if (errno == ENOENT) { // No such file or directory
                        // Let's create it
                        fprintf(stdout, "File \"%s\" doesn't exist, creating it\n", fname.c_str());
                        keycode = 'f';
                        exit = false;
                        continue;
                    }
                    else { // No other way to handle this is provided
                        perror("");
                        fatal_error("File \"%s\" could not be opened.", fname.c_str());
                    }
                }
                loadedKeypoints.release(); loadedKeypoints.reset(ptr); // Set the unique_ptr to ptr
				loadedKeypointsSize = numKeypoints;
                t.logElapsed("load keypoints");
				break;
			case 'd':
				// Go to next image
				// (Nothing to do since i++ will happen in the for-loop update)
				break;
			case 't': // FIXME: This case's code may be broken
				// Show current transformation
				
				// Check preconditions
				if (allTransformations.size() > 0 && firstImage.data != nullptr) {
                    t.reset();
					puts("Showing current transformation");
					
					H = &allTransformations.back();
			
					// //-- Get the corners from the image_1 ( the object to be "detected" )
					// std::vector<cv::Point2f> obj_corners(4);
					// obj_corners[0] = cv::Point2f(0, 0);
					// obj_corners[1] = cv::Point2f( (float)img_object.cols, 0 );
					// obj_corners[2] = cv::Point2f( (float)img_object.cols, (float)img_object.rows );
					// obj_corners[3] = cv::Point2f( 0, (float)img_object.rows );
					// std::vector<cv::Point2f> scene_corners(4);

					// cv::perspectiveTransform( obj_corners, scene_corners, H);

					// //-- Draw lines between the corners of the "object" (the mapped object in the scene - image_2 )
					// cv::line( img_matches, scene_corners[0] + cv::Point2f((float)img_object.cols, 0),
					//       scene_corners[1] + cv::Point2f((float)img_object.cols, 0), cv::Scalar(0, 255, 0), 4 );
					// cv::line( img_matches, scene_corners[1] + cv::Point2f((float)img_object.cols, 0),
					//       scene_corners[2] + cv::Point2f((float)img_object.cols, 0), cv::Scalar( 0, 255, 0), 4 );
					// cv::line( img_matches, scene_corners[2] + cv::Point2f((float)img_object.cols, 0),
					//       scene_corners[3] + cv::Point2f((float)img_object.cols, 0), cv::Scalar( 0, 255, 0), 4 );
					// cv::line( img_matches, scene_corners[3] + cv::Point2f((float)img_object.cols, 0),
					//       scene_corners[0] + cv::Point2f((float)img_object.cols, 0), cv::Scalar( 0, 255, 0), 4 );

					// std::string t1, t2;
					// t1 = type2str(img_matches.type());
					// t2 = type2str(img_object.type());
					// printf("img_matches type: %s, img_object type: %s\n", t1.c_str(), t2.c_str());
					// https://answers.opencv.org/question/54886/how-does-the-perspectivetransform-function-work/
					cv::warpPerspective(img_object, canvas /* <-- destination */, *H /* aka "M" */, img_matches.size());
					imshow(path, canvas);
					keycode = cv::waitKey(0);
					exit = false;
                    t.logElapsed("show current transformation");
				}
				else {
					puts("No transformations or firstImage yet");
				}
				break;
			case 's':
				// Show transformations so far
                t.reset();
                    
				// Check preconditions
				if (allTransformations.size() > 0 && firstImage.data != nullptr) {
					puts("Showing transformation of current image that eventually leads to firstImage");
					// cv::Mat M = allTransformations[0];
					// // Apply all transformations to the first image (future todo: condense all transformations in `allTransformations` into one matrix as we process each image)
					// for (auto it = allTransformations.begin() + 1; it != allTransformations.end(); ++it) {
					// 	M *= *it;
					// }
					if (currentTransformation >= allTransformations.size()) {
						puts("No transformations left");
						imshow(path, canvas);
					}
					else {
						printf("Applying transformation %zu ", currentTransformation);
						M *= allTransformations[currentTransformation].inv();
						cv::Ptr<cv::Formatter> fmt = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
						fmt->set64fPrecision(4);
						fmt->set32fPrecision(4);
						auto s = fmt->format(M);
						std::cout << s << std::endl;
						cv::Mat m = allTransformations[currentTransformation];
						currentTransformation--; // Underflow means we reach a value always >= allTransformations.size() so we are considered finished.
						// FIXME: Maybe the issue here is that the error accumulates because we transform a slightly different image each time?
						// Realized I was using `firstImage` instead of `canvas` below...
						/* // From Google slides:
						  Applied last transformation
						  Seems to zoom in too much (strangely looks like the first image even though this is one transformation based on only two frames of data)
						  Need to control roll more (previous slide shows strange nonlinear motion)

						  Next few slides will be applying the rest of the saved transformations to this image one by one

						 */
						cv::warpPerspective(canvas, canvas /* <-- destination */, m, canvas.size());
						
						//if (counter++ == 0) {
							// Draw first image
							firstImage.copyTo(temp);
							//}

						// Draw the canvas on temp
						canvas.copyTo(temp);

						imshow(path, temp);
					}
					
					keycode = cv::waitKey(0);
					exit = false;
				}
				else {
					puts("No transformations or firstImage yet");
				}
                t.logElapsed("show transformation");
				break;
			}
		} while (!exit);
	
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
    free(params);
	for (struct sift_keypoints* keypoints : computedKeypoints) {
		sift_free_keypoints(keypoints);
	}
	
	return 0;
}
