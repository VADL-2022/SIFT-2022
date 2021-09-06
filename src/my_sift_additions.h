#ifdef __cplusplus
extern "C" {
#endif
  
struct sift_keypoint_std;
struct sift_keypoints;

/** @brief Extracts oriented keypoints (without description
 *  Need to free *outKeypoints with sift_free_keypoints()
 *
 */
struct sift_keypoint_std* my_sift_compute_features(const float* x, int w, int h, int *n, struct sift_keypoints** outKeypoints);

#ifdef __cplusplus
}
#endif
