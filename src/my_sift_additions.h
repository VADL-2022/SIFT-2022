#ifdef __cplusplus
extern "C" {
#endif
  
struct sift_keypoint_std;

/** @brief Extracts oriented keypoints (without description
 *
 *
 */
struct sift_keypoint_std* my_sift_compute_features(const float* x, int w, int h, int *n);

#ifdef __cplusplus
}
#endif
