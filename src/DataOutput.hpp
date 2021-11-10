//
//  DataOutput.hpp
//  SIFT
//
//  Created by VADL on 11/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef DataOutput_hpp
#define DataOutput_hpp

#include <opencv2/opencv.hpp>
#include "common.hpp"
#include "DataSource.hpp"
#include "KeypointsAndMatching.hpp"

template <typename Wrapped>
struct DataOutputBase {
    void output(cv::Mat& m);
    
protected:
    int waitKey(int delay=0);
};

struct PreviewWindowDataOutput : public DataOutputBase<PreviewWindowDataOutput>
{
    void init(size_t width, size_t height);
    void showCanvas(std::string name);
    void showCanvas(std::string name, cv::Mat& canvas);
    
    template <typename DataSourceT>
    void run(DataSourceT& src, SIFTState& s, SIFTParams& p, cv::Mat& backtorgb, struct sift_keypoints* keypoints, bool retryNeeded, size_t& index, int n);
    
    cv::Mat canvas;
    int waitKey(int delay=0);
    protected:
};

struct FileDataOutput : public DataOutputBase<FileDataOutput>
{
    FileDataOutput(double fps = 30, cv::Size sizeFrame = {640,480});
    void run(cv::Mat frame, std::string filenameNoExt);
    
    cv::VideoWriter writer;
    
    cv::Mat canvas;
    
    void showCanvas(std::string name);
    void showCanvas(std::string name, cv::Mat& canvas);
    int waitKey(int delay=0);
protected:
    double fps;
    cv::Size sizeFrame;
};

//#include "DataOutputRun.cpp"

//#define ARGS DataSourceT& src, SIFTState& s, SIFTParams& p, cv::Mat& backtorgb, struct sift_keypoints* keypoints, bool retryNeeded, size_t& index, int n
//// https://stackoverflow.com/questions/6138439/understanding-simple-c-partial-template-specialization : "Partial specialization of a function template['s type parameters], whether it is member function template or stand-alone function template, is not allowed by the Standard"
//template<class T, class DataSourceT>
//void runDataOutput(T& o, ARGS) { throw 0; }
//template<class DataSourceT>
//void runDataOutput<PreviewWindowDataOutput, DataSourceT>(PreviewWindowDataOutput& o, ARGS) {
//    o.run(src, s, p, backtorgb, keypoints, retryNeeded, index, n);
//}
////template<class DataSourceT>
////void runDataOutput<FileDataOutput, DataSourceT>(FileDataOutput& o, ARGS) {
////    //o.run(src, s, p, backtorgb, keypoints, retryNeeded, index, n);
////}
//#undef ARGS


template <typename DataSourceT, typename DataOutputT>
bool run(DataOutputT& o, DataSourceT& src, SIFTState& s, SIFTParams& p, cv::Mat& backtorgb, struct sift_keypoints* keypoints, bool retryNeeded, size_t& index, int n // Number of keypoints
) {
    auto path = src.nameForIndex(index);
    //imshow(path, backtorgb);

    o.showCanvas(path);
    int keycode;
    if (retryNeeded) {
        o.waitKey(1); // Show the window
        keycode = 'a';
    }
    else {
        keycode = o.waitKey(0);
    }
    bool exit;
    size_t currentTransformation = s.allTransformations.size() - 1;
    cv::Mat M = s.allTransformations.size() > 0 ? s.allTransformations[currentTransformation] : cv::Mat();
    cv::Mat* H;
    cv::Mat temp;
    cv::Mat& img_object = backtorgb; // The image containing the "object" (current image)
    cv::Mat& img_matches = backtorgb; // The image on which to draw the lines showing corners of the object (current image)
    std::string fname = src.get(index + 1).empty() /* if out of range */ ? "" : src.nameForIndex(index+1) + ".keypoints.txt";
    std::string fnameMatches = src.get(index + 1).empty() /* if out of range */ ? "" : src.nameForIndex(index+1) + (p.paramsName == nullptr ? "" : (std::string(".") + p.paramsName)) + ".matches" + ".txt";
    struct sift_keypoints* ptr;
    bool skipSavingKeypoints = false;
    do {
        int counter = 0;
        exit = true;
        switch (keycode) {
        case 'q':
            // Quit
            return true;
            break;
        case 'a':
            // Go to previous image
            index -= 2; // FIXME: fix this part, should be decrementing right amount. fix segfault on access previous keypoints too
            // Overwrite prev keypoints
            sift_free_keypoints(s.computedKeypoints.back());
            s.computedKeypoints.pop_back();
            s.computedKeypoints.push_back(keypoints);
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
                        //fatal_error("File \"%s\" could not be opened.", fname.c_str());
                        exit(1);
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
                cv::warpPerspective(img_object, o.canvas /* <-- destination */, *H /* aka "M" */, img_matches.size());
                o.showCanvas(path);
                keycode = o.waitKey(0);
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
                //     M *= *it;
                // }
                if (currentTransformation >= s.allTransformations.size()) {
                    puts("No transformations left");
                    o.showCanvas(path);
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
                    cv::warpPerspective(o.canvas, o.canvas /* <-- destination */, m, o.canvas.size());
                    
                    //if (counter++ == 0) {
                        // Draw first image
                        s.firstImage.copyTo(temp);
                        //}

                    // Draw the canvas on temp
                    o.canvas.copyTo(temp);

                    o.showCanvas(path, temp);
                }
                
                keycode = o.waitKey(0);
                exit = false;
            }
            else {
                puts("No transformations or firstImage yet");
            }
            t.logElapsed("show transformation");
            break;
        }
    } while (!exit);
    
    return false;
}

#endif /* DataOutput_hpp */
