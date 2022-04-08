//
//  SIFT.cpp
//  SIFT
//
//  Created by VADL on 11/14/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "SIFT.hpp"

#include "Config.hpp"

#include "siftMain.hpp"

#ifdef USE_COMMAND_LINE_ARGS
#include "DataSource.hpp"
#endif

#include "../common.hpp"
#include "utils.hpp"

#ifdef SIFTAnatomy_

ProcessedImage<SIFTAnatomy>::ProcessedImage(cv::Mat& image_,
PythonLockedOptional<py::array_t<float>> image_python_,
shared_keypoints_ptr_t computedKeypoints_,
std::shared_ptr<struct sift_keypoint_std> k_,
int n_, // Number of keypoints in `k`
shared_keypoints_ptr_t out_k1_,
shared_keypoints_ptr_t out_k2A_,
shared_keypoints_ptr_t out_k2B_,
cv::Mat transformation_,
SIFTParams p_,
size_t i_,
std::shared_ptr<IMUData> imu_) :
image(image_),
image_python(image_python_),
computedKeypoints(computedKeypoints_),
k(k_),
n(n_),
out_k1(out_k1_),
out_k2A(out_k2A_),
out_k2B(out_k2B_),
transformation(transformation_),
p(p_),
i(i_),
imu(imu_)
{}

std::pair<sift_keypoints* /*keypoints*/, std::pair<sift_keypoint_std* /*k*/, int /*n*/>> SIFTAnatomy::findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale) {
    assert(greyscale.depth() == CV_32F);
    assert(greyscale.type() == CV_32FC1);
    float* x = (float*)greyscale.data;
    size_t w = greyscale.cols, h = greyscale.rows;
    { out_guard();
        std::cout << "width: " << w << ", height: " << h << std::endl; }

    // compute sift keypoints
    t.reset();
    int n; // Number of keypoints
    struct sift_keypoints* keypoints;
    struct sift_keypoint_std *k;
    k = my_sift_compute_features(p.params, x, w, h, &n, &keypoints);
    printf("Thread %d: Number of keypoints: %d\n", threadID, n);
    if (n < 4) {
        printf("Thread %d: Not enough keypoints to find homography! Ignoring this image\n", threadID);
        // TODO: Simply let the transformation be an identity matrix?
        //exit(3);
        //stopMain();
        
//                t.logElapsed(id, "compute features");
//                goto end;
    }
    t.logElapsed(threadID, "compute features");

    return std::make_pair(keypoints, std::make_pair(k, n));
}

void handleNotEnoughDescriptors(ProcessedImage<SIFTAnatomy>& img2) {
    img2.transformation = cv::Mat::eye(3, 3, CV_64F); // 3x3 identity matrix ( https://docs.opencv.org/4.x/d3/d63/classcv_1_1Mat.html )
}

// Finds matching and homography
MatchResult SIFTAnatomy::findHomography(ProcessedImage<SIFTAnatomy>& img1, ProcessedImage<SIFTAnatomy>& img2, SIFTState& s
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
#endif
) {
    t.reset();
    struct sift_keypoints* keypointsPrev = img1.computedKeypoints.get();
    struct sift_keypoints* keypoints = img2.computedKeypoints.get();
    { out_guard();
        std::cout << keypointsPrev << ", " << keypoints << "\n"; }
    assert(keypointsPrev != nullptr && keypoints != nullptr); // This should always hold, since the other thread pool threads enqueued them.
    assert(img2.out_k1 == nullptr && img2.out_k2A == nullptr && img2.out_k2B == nullptr); // This should always hold since we reset the pointers when done with matching
    img2.out_k1.reset(sift_malloc_keypoints());
    img2.out_k2A.reset(sift_malloc_keypoints());
    img2.out_k2B.reset(sift_malloc_keypoints());
    if (keypointsPrev->size == 0 || keypoints->size == 0) {
        std::cout << "Matcher thread: zero keypoints, cannot match. Saving identity transformation.\n";
        //throw ""; // TODO: temp, need to notify main thread and retry matching maybe
        return MatchResult::NotEnoughKeypoints;
    }
    matching(keypointsPrev, keypoints, img2.out_k1.get(), img2.out_k2A.get(), img2.out_k2B.get(), img2.p.thresh, img2.p.meth_flag); // NOTE: there can be less descriptors in out_k1 etc. than in keypoints->size or the n value.
    t.logElapsed("Matcher thread: find matches");
    
    // Find homography
    t.reset();
    struct sift_keypoints* k1 = img2.out_k1.get();
    struct sift_keypoints* out_k2A = img2.out_k2A.get();
    struct sift_keypoints* out_k1 = k1;
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
    bool notEnoughSecond = obj.size() < 4;
    bool notEnoughFirst = scene.size() < 4;
    if (notEnoughSecond || notEnoughFirst) { // Prevents "libc++abi.dylib: terminating with uncaught exception of type cv::Exception: OpenCV(4.5.2) /tmp/nix-build-opencv-4.5.2.drv-0/source/modules/calib3d/src/fundam.cpp:385: error: (-28:Unknown error code -28) The input arrays should have at least 4 corresponding point sets to calculate Homography in function 'findHomography'"
        //printf("Not enough keypoints to find homography! Trying to find keypoints on previous image again with tweaked params\n");
//            // Retry with tweaked params
//            img2.p.params->n_oct++;
//            img2.p.params->n_bins++;
//            img2.p.params->n_hist++;
//            if (img2.p.params->n_spo < 2) {
//                img2.p.params->n_spo = 2;
//            }
//            img2.p.params->sigma_min -= 0.05;
//            img2.p.params->delta_min -= 0.05;
//            img2.p.params->sigma_in -= 0.05;
        
        //throw "Not yet implemented"; // This is way too complex..
        
        
        //printf("Not enough keypoints to find homography! Ignoring this image\n");
        //goto end;
        
        MatchResult retval = notEnoughFirst ? MatchResult::NotEnoughMatchesForFirstImage : MatchResult::NotEnoughMatchesForSecondImage;
        printf("Matcher thread: not enough descriptors in %s to find homography. Saving identity transformation.\n", retval == MatchResult::NotEnoughMatchesForFirstImage ? "first image" : "second image");
        return retval;
    }
    
    bool affine = true;
    int method = cv::LMEDS /*cv::RANSAC*/;
    img2.transformation = affine ? cv::estimateAffine2D(obj, scene, cv::noArray(), method) : cv::findHomography( obj, scene, method );
    if (img2.transformation.empty()) { // "Note that whenever an H matrix cannot be estimated, an empty one will be returned." ( https://docs.opencv.org/3.3.0/d9/d0c/group__calib3d.html#ga4abc2ece9fab9398f2e560d53c8c9780 , https://stackoverflow.com/questions/28331296/opencv-findhomography-generating-an-empty-matrix )
        std::cout << "cv::findHomography returned empty matrix, indicating that a matrix could not be estimated.\n";
        return MatchResult::NotEnoughMatchesForFirstImage; // Optimistic
    }
    if (affine) { // Make the affine transformation into a perspective one: "Since affine transformations can be thought of as homographies where the bottom row is 0, 0, 1, affine transformations are still homographies. See affine homography on Wiki for e.g.. If someone says "homography" or "perspective transformation" though they mean a 3x3 transformation." ( https://stackoverflow.com/questions/45637472/opencv-transformationmatrix-affine-vs-perspective-warping )
        cv::Mat_<double> row = (cv::Mat_<double>(1, 3) << 0, 0, 1); // https://github.com/opencv/opencv/blob/master/modules/core/include/opencv2/core.hpp
        img2.transformation.push_back(row);
        // Or: cv::vconcat(img2.transformation, row);
        
        cv::Ptr<cv::Formatted> str;
        matrixToString(img2.transformation, str);
        { out_guard();
            std::cout << "affine made into perspective matrix: " << str << std::endl; }
    }

    printf("Number of matching keypoints: %d\n", k1->size);
    if (CMD_CONFIG(mainMission)) {
#ifdef USE_COMMAND_LINE_ARGS
        // Draw it //
        cv::Mat backtorgb = src->colorImageForMat(img2.i);
        backtorgb.copyTo(img2.canvas);

        auto xoff=0;//100;
        auto yoff=-100;
        cv::Rect crop(0,0,backtorgb.cols,backtorgb.rows); // Initial value
        if (src->shouldCrop()) {
            crop = src->crop();
            xoff += crop.x / 2;
            yoff += crop.y / 2;
        }
        
        // Draw keypoints on `img2.canvas`
        struct sift_keypoint_std *k = img2.k.get();
        int n = img2.n;
        t.reset();
        // Draw current crop
        //drawRect(img2.canvas, {crop.x, crop.y}, crop.size(), 0);
        for(int i=0; i<n; i++){
            if (!CMD_CONFIG(noDots)) {
                drawSquare(img2.canvas, cv::Point(k[i].x+xoff, k[i].y+yoff), k[i].scale, k[i].orientation, 1);
            }
            //break;
            // fprintf(f, "%f %f %f %f ", k[i].x, k[i].y, k[i].scale, k[i].orientation);
            // for(int j=0; j<128; j++){
            //     fprintf(f, "%u ", k[i].descriptor[j]);
            // }
            // fprintf(f, "\n");
        }
        t.logElapsed("draw keypoints");
        
        // Draw matches
        t.reset();
        auto& s = img2;
        struct sift_keypoints* k1 = s.out_k1.get();
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
                
                if (!CMD_CONFIG(noDots)) {
                    drawSquare(img2.canvas, cv::Point(s.out_k2A->list[i]->x+xoff, s.out_k2A->list[i]->y+yoff), s.out_k2A->list[i]->sigma /* need to choose something better here */, s.out_k2A->list[i]->theta, 2);
                }
                if (!CMD_CONFIG(noLines)) {
                    cv::line(img2.canvas, cv::Point(s.out_k1->list[i]->x+xoff, s.out_k1->list[i]->y+yoff), cv::Point(s.out_k2A->list[i]->x+xoff, s.out_k2A->list[i]->y+yoff), lastColor, 1);
                }
            }
        }
        t.logElapsed("draw matches");
        
        t.reset();
        // Reset RNG so some colors coincide
        resetRNG();
        t.logElapsed("reset RNG");
        // //
#endif
    }
    
    return MatchResult::Success;
}

#elif defined(SIFTOpenCV_)
std::pair<std::vector<cv::KeyPoint>, cv::Mat /*descriptors*/> SIFTOpenCV::findKeypoints(int threadID, SIFTParams& p, cv::Mat& greyscale) {
//    t.reset();
//    auto ret = detect(greyscale);
//    t.logElapsed(threadID, "compute features");
//    t.reset();
//    auto ret2 = descriptors(greyscale, ret);
//    t.logElapsed(threadID, "compute descriptors");
//    return std::make_pair(ret, ret2);

    t.reset();
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    f2d->detectAndCompute(greyscale, cv::Mat(), keypoints, descriptors);
    
    printf("Thread %d: Number of keypoints: %zu\n", threadID, keypoints.size());
    if (keypoints.size() < 4) {
        printf("Not enough keypoints to find homography! Ignoring this image\n");
        // TODO: Simply let the transformation be an identity matrix?
        //exit(3);
    }
    t.logElapsed(threadID, "compute features");
    
    return std::make_pair(keypoints, descriptors);
}

void SIFTOpenCV::findHomography(ProcessedImage<SIFTOpenCV>& img1, ProcessedImage<SIFTOpenCV>& img2, SIFTState& s
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
#endif
) {
    img2.matches = match(img1.descriptors, img2.descriptors);
    
    // Using descriptors (makes it able to match each feature using scale invariance):
    // https://stackoverflow.com/questions/13318853/opencv-drawmatches-queryidx-and-trainidx , https://stackoverflow.com/questions/30716610/how-to-get-pixel-coordinates-from-feature-matching-in-opencv-python
    std::vector<cv::Point2f> obj; obj.reserve(img2.matches.size());
    std::vector<cv::Point2f> scene; obj.reserve(img1.matches.size()); // TODO: correct order?
    for (cv::DMatch match : img2.matches) {
        scene.push_back(img1.computedKeypoints[match.queryIdx].pt);
        obj.push_back(img2.computedKeypoints[match.trainIdx].pt);
    }
    
    // Basic:
//    std::vector<cv::Point2f> obj;
//    std::vector<cv::Point2f> scene;
//    cv::KeyPoint::convert(img2.computedKeypoints, obj);
//    cv::KeyPoint::convert(img1.computedKeypoints, scene); // TODO: correct order?
//    img2.transformation = cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ );
    
    // Better:
    img2.transformation = cv::findHomography( obj, scene, cv::LMEDS /*cv::RANSAC*/ );
    if (img2.transformation.empty()) { // "Note that whenever an H matrix cannot be estimated, an empty one will be returned." ( https://docs.opencv.org/3.3.0/d9/d0c/group__calib3d.html#ga4abc2ece9fab9398f2e560d53c8c9780 , https://stackoverflow.com/questions/28331296/opencv-findhomography-generating-an-empty-matrix )
        std::cout << "cv::findHomography returned empty matrix, indicating that a matrix could not be estimated.\n";
        return MatchResult::NotEnoughMatchesForFirstImage; // Optimistic
    }
    
    if (CMD_CONFIG(mainMission)) {
#ifdef USE_COMMAND_LINE_ARGS
        // Make it fill the screen
        cv::Mat img1Resized, img2Resized;
        cv::resize(img1.image, img1Resized, g_desktopSize.size());
        cv::resize(img2.image, img2Resized, g_desktopSize.size());
        //if (!img2.canvas.empty()) {
            // Get scale to destination
            cv::Size2d scale = cv::Size2d((double)g_desktopSize.size().width / img2.image.cols, (double)g_desktopSize.size().height / img2.image.rows);
            { out_guard();
                std::cout << "Scale: " << scale << "\n"; }
            float max = std::max(scale.width, scale.height);
            
            // Resize canvas
            //cv::resize(img2.canvas, img2.canvas, g_desktopSize.size(), 0, 0, cv::INTER_LINEAR);
            // Scale all keypoints
            auto s = [&](std::vector<cv::KeyPoint>& computedKeypoints) {
                for (cv::KeyPoint& p : computedKeypoints) {
                    p.pt.x *= scale.width;
                    p.pt.y *= scale.height;
                    p.size *= max;
                }
            };
            s(img1.computedKeypoints);
            s(img2.computedKeypoints);
        //}
        
        // Draw it
        // Side-by-side matches: //
//        cv::drawMatches(img1Resized, img1.computedKeypoints, img2Resized, img2.computedKeypoints, img2.matches, img2.canvas);
        // //
        // Single image //
        img2Resized.copyTo(img2.canvas);
        for (auto& match : img2.matches) {
            cv::KeyPoint& p1 = img1.computedKeypoints[match.trainIdx];
            cv::KeyPoint& p2 = img2.computedKeypoints[match.queryIdx];

            //const int draw_shift_bits = 4;
            //const int draw_multiplier = 1 << draw_shift_bits;
            cv::Point center1( cvRound(p1.pt.x), cvRound(p1.pt.y) ); // TODO: FIXME: With address sanitizer, this says "Heap buffer overflow"
            cv::Point center2( cvRound(p2.pt.x), cvRound(p2.pt.y) );
            drawCircle(img2.canvas, center1, 2);
            drawCircle(img2.canvas, center2, 2);
            cv::line(img2.canvas, center1, center2, lastColor, 1);
        }
        // //
#endif
    }
}

#elif defined(SIFTGPU_)
#include <GL/glew.h>

#ifdef SIFTGPU_TEST
int SIFTGPU::test(int argc, char** argv) {
    SiftGPU sift;
    
    //Parse parameters
    sift.ParseParam(argc - 1, argv + 1);
    //sift.SetVerbose(0);
    sift.SetVerbose();
    
    std::cout<<"Initialize and warm up...\n";
    //create an OpenGL context for computation
    if(sift.CreateContextGL() ==0) {
        std::cout << "CreateContextGL() failed" << std::endl;
        return 1;
    }
    // Warm up
    if(sift.RunSIFT()==0) { // Should fail due to _image_loaded == 0 (we didn't load any images)
        std::cout << "RunSIFT() failed" << std::endl;
        return 1;
    }

    sift.RunSIFT();
    
    return 0;
}
#endif

std::pair<std::vector<SiftGPU::SiftKeypoint> /*keys1*/, std::pair<std::vector<float> /*descriptors1*/, int /*num1*/>> SIFTGPU::findKeypoints(int threadID, SIFTState& s, SIFTParams& p, cv::Mat& greyscale) {
    if (s.sift.RunSIFT(greyscale.cols, greyscale.rows, greyscale.data, GL_LUMINANCE, GL_UNSIGNED_BYTE) == 0) { // Greyscale gl types (see SiftGPU/src/SiftGPU/GLTexImage.h under `static int  IsSimpleGlFormat(unsigned int gl_format, unsigned int gl_type)`)
        { out_guard();
            std::cout << "RunSIFT() failed" << std::endl; }
        throw ""; // TODO: don't throw
    }
    
    std::vector<float > descriptors1(1), descriptors2(1);
    std::vector<SiftGPU::SiftKeypoint> keys1(1), keys2(1);
    int num1 = 0, num2 = 0;
    
    //get feature count
    num1 = s.sift.GetFeatureNum();

    //allocate memory
    keys1.resize(num1);    descriptors1.resize(128*num1);

    //reading back feature vectors is faster than writing files
    //if you dont need keys or descriptors, just put NULLs here
    s.sift.GetFeatureVector(&keys1[0], &descriptors1[0]);
    
    return std::make_pair(keys1, std::make_pair(descriptors1, num1));
}

template <typename T>
struct DeleteArray {
    static void action(T p) {
        delete[] p;
    }
};
template <typename F, typename T>
struct ScopedAction {
    void* userdata;
    ScopedAction(void* userdata_) : userdata(userdata_) {}
    ~ScopedAction() {
        F::action((T)userdata);
    }
};

void SIFTGPU::findHomography(ProcessedImage<SIFTGPU>& img1, ProcessedImage<SIFTGPU>& img2, SIFTState& s
#ifdef USE_COMMAND_LINE_ARGS
    , DataSourceBase* src, CommandLineConfig& cfg
#endif
) {
    SiftMatchGPU* matcher = &s.matcher;
    auto& keys1 = img1.keys1;
    auto& keys2 = img2.keys1;
    auto& descriptors1 = img1.descriptors1;
    auto& descriptors2 = img2.descriptors1;
    auto& num1 = img1.num1;
    auto& num2 = img2.num1;
    
    //Verify current OpenGL Context and initialize the Matcher;
    //If you don't have an OpenGL Context, call matcher->CreateContextGL instead;
    matcher->VerifyContextGL(); //must call once

    //Set descriptors to match, the first argument must be either 0 or 1
    //if you want to use more than 4096 or less than 4096
    //call matcher->SetMaxSift() to change the limit before calling setdescriptor
    matcher->SetDescriptors(0, num1, &descriptors1[0]); //image 1
    matcher->SetDescriptors(1, num2, &descriptors2[0]); //image 2

    //match and get result.
    int (*match_buf)[2] = new int[num1][2];
    using T = decltype(match_buf);
    using Freer = DeleteArray<T>;
    ScopedAction<Freer, T> freer(match_buf);
    //use the default thresholds. Check the declaration in SiftGPU.h
    int num_match = matcher->GetSiftMatch(num1, match_buf);
    { out_guard();
        std::cout << num_match << " sift matches were found;\n"; }
    
    //enumerate all the feature matches
//    for(int i  = 0; i < num_match; ++i)
//    {
//        //How to get the feature matches:
//        SiftGPU::SiftKeypoint & key1 = keys1[match_buf[i][0]];
//        SiftGPU::SiftKeypoint & key2 = keys2[match_buf[i][1]];
//        //key1 in the first image matches with key2 in the second image
//    }
    
    // TODO: maybe speed up by not copying, maybe using a stride instead since we have 2 floats x and y in the SiftGPU::SiftKeypoint but then 2 extra floats (see its definition). cv::Point2f has 2 floats and we would need to match that data layout.
//    auto& scene = keys1;
//    auto& obj = keys2;
    
    std::vector<cv::Point2f> obj; // The `obj` is the current image's keypoints
    std::vector<cv::Point2f> scene;
    obj.reserve(keys2.size());
    for (size_t i = 0; i < keys2.size(); i++) {
        obj.emplace_back(keys2[i].x, keys2[i].y);
    }
    scene.reserve(keys1.size());
    for (size_t i = 0; i < keys1.size(); i++) {
        obj.emplace_back(keys1[i].x, keys1[i].y);
    }
    
    img2.transformation = cv::findHomography(obj, scene, cv::LMEDS /*cv::RANSAC*/ );
    if (img2.transformation.empty()) { // "Note that whenever an H matrix cannot be estimated, an empty one will be returned." ( https://docs.opencv.org/3.3.0/d9/d0c/group__calib3d.html#ga4abc2ece9fab9398f2e560d53c8c9780 , https://stackoverflow.com/questions/28331296/opencv-findhomography-generating-an-empty-matrix )
        std::cout << "cv::findHomography returned empty matrix, indicating that a matrix could not be estimated.\n";
        return MatchResult::NotEnoughMatchesForFirstImage; // Optimistic
    }

    if (CMD_CONFIG(mainMission)) {
#ifdef USE_COMMAND_LINE_ARGS
        // Draw it //
        cv::Mat backtorgb = src->colorImageForMat(img2.i);
        backtorgb.copyTo(img2.canvas);
        
        // Draw keypoints on `img2.canvas`
        t.reset();
        for(int i=0; i<keys2.size(); i++){
            drawSquare(img2.canvas, cv::Point(keys2[i].x, keys2[i].y), keys2[i].s, keys2[i].o, 1);
        }
        t.logElapsed("draw keypoints");
        
        // Draw matches
        t.reset();
        for(int i = 0; i < num_match; i++){
            SiftGPU::SiftKeypoint & key1 = keys1[match_buf[i][0]];
            SiftGPU::SiftKeypoint & key2 = keys2[match_buf[i][1]];
            //key1 in the first image matches with key2 in the second image
            
            auto xoff=0;//100;
            auto yoff=0;//-100;
//            drawSquare(img2.canvas, cv::Point(key2.x+xoff, key2.y+yoff), key2.sigma /* need to choose something better here */, key2.theta, 2); // <-- Note: sigma and theta are not defined for keypoint struct in SiftGPU, need an alternative
            cv::line(img2.canvas, cv::Point(key1.x+xoff, key1.y+yoff), cv::Point(key2.x+xoff, key2.y+yoff), lastColor, 1);
        }
        t.logElapsed("draw matches");
        
        t.reset();
        // Reset RNG so some colors coincide
        resetRNG();
        t.logElapsed("reset RNG");
        // //
#endif
    }
}

#endif
