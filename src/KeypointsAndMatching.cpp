#include "KeypointsAndMatching.hpp"

SIFTParams::SIFTParams() {
    /** assign parameters **/
    params = sift_assign_default_parameters();
    v3Params(params);
    //v2Params(params);
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
