//
//  KeypointsAndMatching.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef KeypointsAndMatching_hpp
#define KeypointsAndMatching_hpp

#include "Includes.hpp"

struct SIFTParams {
    #define USE(paramsFunc) { p.paramsName = #paramsFunc; paramsFunc(p.params); }
    #define DeclareParamsFunc(p) { #p, p },
    // https://stackoverflow.com/questions/1118705/call-a-function-named-in-a-string-variable-in-c
    constexpr const static struct {
      const char *name;
      void (*func)(struct sift_parameters*);
    } function_map [] = {
        DeclareParamsFunc(v1Params)
        DeclareParamsFunc(v2Params)
        DeclareParamsFunc(v3Params)
        
    //  { "function_a", function_a },
    //  { "function_b", function_b },
    //  { "function_c", function_c },
    //  { "function_d", function_d },
    //  { "function_e", function_e },
    };
    #undef DeclareParamsFunc
    // Returns -1 if not found, 0 if called successfully.
    static int call_params_function(const char *name, struct sift_parameters* p);
    static void print_params_functions();
    
    
    SIFTParams();
    ~SIFTParams();
    
    // Set params //
    struct sift_parameters* params;
    const char* paramsName = nullptr; // Name to identify these set of params, if any
    bool checkLoadedK = true;
    
    // For matching //
    int meth_flag;
    float thresh;
    
    // Loaded params:
    int loaded_meth_flag;
    float loaded_thresh;
    // //
    // //
};

// https://stackoverflow.com/questions/19053351/how-do-i-use-a-custom-deleter-with-a-stdunique-ptr-member/19054280 //
template <auto fn>
using deleter_from_fn = std::integral_constant<decltype(fn), fn>;

template <typename T, auto fn>
using unique_ptr_with_deleter = std::unique_ptr<T, deleter_from_fn<fn>>;
// //

using unique_keypoints_ptr = unique_ptr_with_deleter<struct sift_keypoints, sift_free_keypoints>;

struct SIFTState {
    ~SIFTState();
    
    std::vector<struct sift_keypoints*> computedKeypoints;
    std::vector<cv::Mat> allTransformations;
    cv::Mat firstImage;
    
    // Loaded keypoints:
    unique_keypoints_ptr loadedKeypoints;
    std::unique_ptr<struct sift_keypoint_std> loadedK;
    int loadedKeypointsSize = 0;
    
    // For matching //
    struct sift_keypoints* out_k1 = nullptr;
    struct sift_keypoints* out_k2A = nullptr;
    struct sift_keypoints* out_k2B = nullptr;
    
    // Loaded matches
    unique_keypoints_ptr loaded_out_k1 = nullptr;
    unique_keypoints_ptr loaded_out_k2A = nullptr;
    unique_keypoints_ptr loaded_out_k2B = nullptr;
    // //
};

#endif /* KeypointsAndMatching_hpp */
