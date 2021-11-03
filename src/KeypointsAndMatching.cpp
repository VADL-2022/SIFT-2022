//
//  KeypointsAndMatching.cpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "KeypointsAndMatching.hpp"

#define USE(paramsFunc) { paramsName = #paramsFunc; paramsFunc(params); }
SIFTParams::SIFTParams() {
    /** assign parameters **/
    params = sift_assign_default_parameters();
    //USE(v3Params);
    USE(v2Params);
    // //
}

SIFTParams::~SIFTParams() {
    // Cleanup
    free(params);
}

SIFTState::~SIFTState() {
    // Cleanup
    for (struct sift_keypoints* keypoints : computedKeypoints) {
        sift_free_keypoints(keypoints);
    }

    if (out_k1 != nullptr) {
        sift_free_keypoints(out_k1);
        sift_free_keypoints(out_k2A);
        sift_free_keypoints(out_k2B);
    }
}
