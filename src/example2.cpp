#include "common.hpp"

#include "KeypointsAndMatching.hpp"
#include "compareKeypoints.hpp"

// https://docs.opencv.org/master/da/d6a/tutorial_trackbar.html
const int alpha_slider_max = 2;
int alpha_slider = 0;
double alpha;
double beta;
static void on_trackbar( int, void* )
{
   alpha = (double) alpha_slider/alpha_slider_max ;
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
		if (endsWith(entry.path().string(), ".keypoints.txt") || endsWith(entry.path().string(), ".matches.txt")) {
			continue;
		}
		files.push_back(entry.path().string());
	}

	// https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
	std::sort(files.begin(),files.end(),compareNat);
	
	//--- print the files sorted by filename
    SIFTState s;
    SIFTParams p;
	for (size_t i = skip; i < files.size(); i++) {
		//for (size_t i = files.size() - 1 - skip; i < files.size() /*underflow of i will end the loop*/; i--) {
		auto& path = files[i];
		std::cout << path << std::endl;

		// Loading image
        t.reset();
		size_t w, h;
        errno = ESUCCESS; // Set errno so we can read it again to check why any errors happen after the below call
		float* x = io_png_read_f32_gray(path.c_str(), &w, &h);
        t.logElapsed("load image");
        if (x == nullptr) {
            if (errno != ESUCCESS) {
                perror("");
                fatal_error("File \"%s\" could not be opened", path.c_str());
            }
            else {
                fatal_error("File \"%s\" cannot be loaded as an image", path.c_str());
            }
        }
        t.reset();
		for(int i=0; i < w*h; i++)
			x[i] /=256.; // TODO: why do we do this?
        t.logElapsed("normalize image");

		// Initialize canvas if needed
        if (s.canvas.data == nullptr) {
			puts("Init canvas");
			s.canvas = cv::Mat(h, w, CV_32FC4);
		}

		// compute sift keypoints
		int n; // Number of keypoints
		struct sift_keypoints* keypoints;
		struct sift_keypoint_std *k;
		if (s.loadedKeypoints == nullptr) {
            t.reset();
            k = my_sift_compute_features(p.params, x, w, h, &n, &keypoints);
            t.logElapsed("compute features");
		}
		else {
            k = s.loadedK.release(); // "Releases the ownership of the managed object if any." ( https://en.cppreference.com/w/cpp/memory/unique_ptr/release )
            keypoints = s.loadedKeypoints.release();
            n = s.loadedKeypointsSize;
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
            s.firstImage = backtorgb;
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
        bool retryNeeded = compareKeypoints(s, p, keypoints, backtorgb);
		
		//imshow(path, backtorgb);

        t.reset();
        imshow(path, s.canvas);
        t.logElapsed("show canvas window");
        int keycode;
        if (retryNeeded) {
            cv::waitKey(1); // Show the window
            keycode = 'a';
        }
        else {
            keycode = cv::waitKey(0);
        }
		bool exit;
        size_t currentTransformation = s.allTransformations.size() - 1;
		cv::Mat M = s.allTransformations.size() > 0 ? s.allTransformations[currentTransformation] : cv::Mat();
		cv::Mat* H;
		cv::Mat temp;
		cv::Mat& img_object = backtorgb; // The image containing the "object" (current image)
		cv::Mat& img_matches = backtorgb; // The image on which to draw the lines showing corners of the object (current image)
		std::string fname = i + 1 >= files.size() ? "" : files[i+1] + ".keypoints.txt";
        std::string fnameMatches = i + 1 >= files.size() ? "" : files[i+1] + ".matches.txt";
        struct sift_keypoints* ptr;
        bool skipSavingKeypoints = false;
		do {
			int counter = 0;
			exit = true;
			switch (keycode) {
			case 'a':
				// Go to previous image
				i -= 2; // FIXME: fix this part, should be decrementing right amount
				break;
			case 'f':
				// 'f' for "file" writing
				// Go to next image and save SIFT keypoints
				
				// Serialize to file
                if (!skipSavingKeypoints) {
                    t.reset();
                    printf("Saving keypoints to %s and matches to %s\n", fname.c_str(), fnameMatches.c_str());
                    my_sift_write_to_file(fname.c_str(), keypoints, p.params, n); //sift_write_to_file(fname.c_str(), k, n); //<--not used because it doesn't save params
                    t.logElapsed("save keypoints");
                }
                else {
                    skipSavingKeypoints = false;
                }
                if (s.out_k1 != nullptr) {
                    // Save matches
                    t.reset();
                    my_sift_write_matches_to_file(fnameMatches.c_str(), s.out_k1, s.out_k2A, s.out_k2B, p.params, p.meth_flag, p.thresh); // TODO: dont save the newly made params since they might be loaded already and then written again. or maybe keep params separate..
                    t.logElapsed("save matches");
                }
                else {
                    printf("(No matches can be saved for the first image since it has no previous image to match to)\n");
                }
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
                printf("Loading keypoints from next image's keypoints file %s and matches from %s\n", fname.c_str(), fnameMatches.c_str()); // TODO: load matches
                ptr = s.loadedKeypoints.get(); // Make regular pointer so we can use a double pointer below
                s.loadedK.reset(my_sift_read_from_file(fname.c_str(), &numKeypoints, &ptr)); // https://en.cppreference.com/w/cpp/memory/unique_ptr/reset
                if (s.loadedK == nullptr) {
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
                s.loadedKeypoints.release(); s.loadedKeypoints.reset(ptr); // Set the unique_ptr to ptr
				s.loadedKeypointsSize = numKeypoints;
                t.logElapsed("load keypoints");
                // Load matches
                if (s.out_k1 != nullptr) {
                    t.reset();
                    s.loaded_out_k1.reset(sift_malloc_keypoints());
                    s.loaded_out_k2A.reset(sift_malloc_keypoints());
                    s.loaded_out_k2B.reset(sift_malloc_keypoints());
                    if (my_sift_read_matches_from_file(fnameMatches.c_str(), s.loaded_out_k1.get(), s.loaded_out_k2A.get(), s.loaded_out_k2B.get(), &p.loaded_meth_flag, &p.loaded_thresh) == 1) {
                        // Failed to read the file
                        if (errno == ENOENT) { // No such file or directory
                            // Let's create it
                            fprintf(stdout, "File \"%s\" doesn't exist, creating it\n", fname.c_str());
                            keycode = 'f';
                            skipSavingKeypoints = true;
                            exit = false;
                            continue;
                        }
                        else { // No other way to handle this is provided
                            perror("");
                            fatal_error("File \"%s\" could not be opened.", fname.c_str());
                        }
                    }
                    t.logElapsed("load matches");
                }
				break;
			case 'd':
				// Go to next image
				// (Nothing to do since i++ will happen in the for-loop update)
				break;
			case 't': // FIXME: This case's code may be broken
				// Show current transformation
				
				// Check preconditions
				if (s.allTransformations.size() > 0 && s.firstImage.data != nullptr) {
                    t.reset();
					puts("Showing current transformation");
					
					H = &s.allTransformations.back();
			
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
					cv::warpPerspective(img_object, s.canvas /* <-- destination */, *H /* aka "M" */, img_matches.size());
					imshow(path, s.canvas);
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
				if (s.allTransformations.size() > 0 && s.firstImage.data != nullptr) {
					puts("Showing transformation of current image that eventually leads to firstImage");
					// cv::Mat M = allTransformations[0];
					// // Apply all transformations to the first image (future todo: condense all transformations in `allTransformations` into one matrix as we process each image)
					// for (auto it = allTransformations.begin() + 1; it != allTransformations.end(); ++it) {
					// 	M *= *it;
					// }
					if (currentTransformation >= s.allTransformations.size()) {
						puts("No transformations left");
						imshow(path, s.canvas);
					}
					else {
						printf("Applying transformation %zu ", currentTransformation);
						M *= s.allTransformations[currentTransformation].inv();
						cv::Ptr<cv::Formatter> fmt = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
						fmt->set64fPrecision(4);
						fmt->set32fPrecision(4);
						auto str = fmt->format(M);
						std::cout << str << std::endl;
						cv::Mat m = s.allTransformations[currentTransformation];
						currentTransformation--; // Underflow means we reach a value always >= allTransformations.size() so we are considered finished.
						// FIXME: Maybe the issue here is that the error accumulates because we transform a slightly different image each time?
						// Realized I was using `firstImage` instead of `canvas` below...
						/* // From Google slides:
						  Applied last transformation
						  Seems to zoom in too much (strangely looks like the first image even though this is one transformation based on only two frames of data)
						  Need to control roll more (previous slide shows strange nonlinear motion)

						  Next few slides will be applying the rest of the saved transformations to this image one by one

						 */
						cv::warpPerspective(s.canvas, s.canvas /* <-- destination */, m, s.canvas.size());
						
						//if (counter++ == 0) {
							// Draw first image
                            s.firstImage.copyTo(temp);
							//}

						// Draw the canvas on temp
                        s.canvas.copyTo(temp);

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
        s.computedKeypoints.push_back(keypoints);
		
		// Reset RNG so some colors coincide
		resetRNG();
	}
	
	return 0;
}
