//
//  my_sift_additions.h
//  SIFT
//
//  Created by VADL on 9/5/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
  
struct sift_keypoint_std;
struct sift_keypoints;
struct sift_parameters;
  

void v1Params(struct sift_parameters* p);
// White background high contrast
void v2Params(struct sift_parameters* p);
// Based off v1 params, similar
void v3Params(struct sift_parameters* p);
  
void lowEdgeParams(struct sift_parameters* p);

/** @brief Extracts oriented keypoints (without description
 *  Need to free *outKeypoints with sift_free_keypoints()
 *
 */
struct sift_keypoint_std* my_sift_compute_features(struct sift_parameters* p, const float* x, int w, int h, int *n, struct sift_keypoints** outKeypoints);

  void my_fprintf_parameters(FILE* file, struct sift_parameters* params);
void my_fprintf_matching_parameters(FILE* file,
int meth_flag,
                                    float thresh);
  void my_sift_write_to_file(const char *filename, const struct sift_keypoints *keypoints, struct sift_parameters* params, int n);
  void my_sift_load_parameters(FILE* stream, char* buffer, int buffer_size, struct sift_parameters* outParams, int* outVersion);
  struct sift_keypoint_std* my_sift_read_from_file(const char *filename, int *n, struct sift_keypoints** outKeypoints);
  int my_sift_read_matches_from_file(const char *filename,
                                                           // Output:
                                                           const struct sift_keypoints *k1,
                                                           const struct sift_keypoints *k2A,
                                                           const struct sift_keypoints *k2B,
                                     int* meth_flag,
                                     float* thresh);
  void my_sift_read_keypoints(struct sift_keypoints* keys,
                         FILE* stream,
                            char* buffer, // size_t buffer_size = 1024 * 1024;  // 1MB buffer for long lines.
                                          // char* buffer = xmalloc(buffer_size);
                                          // <this function runs>
                                          // xfree(buffer);
                            size_t buffer_size,
                         int n_hist,
                         int n_ori,
                         int n_bins,
			      int flag);

void my_sift_read_one_keypoint(struct sift_keypoints* keys,
                               int* pos,
                               int* read,
                               char* buffer,
                               size_t buffer_size,
                               int n_hist,
                               int n_ori,
                               int n_bins,
                               int flag);
void my_sift_read_matches(
                          // Output:
                          const struct sift_keypoints *k1,
                          const struct sift_keypoints *k2A,
                          const struct sift_keypoints *k2B,
                          // Input:
                          FILE* stream,
                            char* buffer, // size_t buffer_size = 1024 * 1024;  // 1MB buffer for long lines.
                                          // char* buffer = xmalloc(buffer_size);
                                          // <this function runs>
                                          // xfree(buffer);
                            size_t buffer_size,
                         int n_hist,
                         int n_ori,
                         int n_bins,
                          int flag);
void my_sift_write_matches_to_file(const char *filename,
const struct sift_keypoints *k1,
const struct sift_keypoints *k2A,
const struct sift_keypoints *k2B,
struct sift_parameters* params,
int meth_flag,
                                   float thresh);
  void my_save_pairs_extra(FILE* stream,
                        const struct sift_keypoints *k1,
                        const struct sift_keypoints *k2A,
                           const struct sift_keypoints *k2B);

#ifdef __cplusplus
}
#endif
