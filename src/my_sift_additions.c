//
//  my_sift_additions.c
//  SIFT
//
//  Created by VADL on 9/5/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "my_sift_additions.h"

#include <lib_sift.h>
#include <lib_sift_anatomy.h>
#include <lib_util.h>

//void defaultParams(struct sift_parameters* p) {
    // Defaults:
    /* p->n_oct = 8; */
    /* p->n_spo = 3; */
    /* p->sigma_min = 0.8; */
    /* p->delta_min = 0.5; */
    /* p->sigma_in = 0.5; */
    /* p->C_DoG = 0.013333333;  // = 0.04/3 */
    /* p->C_edge = 10; */
    /* p->n_bins = 36; */
    /* p->lambda_ori = 1.5; */
    /* p->t = 0.80; */
    /* p->n_hist = 4; */
    /* p->n_ori = 8; */
    /* p->lambda_descr = 6; */
    /* p->itermax = 5; */
//}

void v1Params(struct sift_parameters* p) {
    p->n_oct = 2;
    p->n_spo = 1;
    p->delta_min=1;
    p->sigma_in=1.1;
    p->sigma_min=1.1;
}
// White background high contrast
void v2Params(struct sift_parameters* p) {
    // Lots of points show up with these parameters:
    p->n_oct = 10;
    p->n_spo = 2;
    p->sigma_min = 0.6;
    p->delta_min = 0.4;
    p->sigma_in = 0.5;
    p->C_DoG = 0.023333333;  // = 0.04/3
    p->C_edge = 10;
    p->n_bins = 36;
    p->lambda_ori = 1.5;
    p->t = 0.80;
    p->n_hist = 4;
    p->n_ori = 8;
    p->lambda_descr = 6;
    p->itermax = 5;
}
// Based off v1 params, similar
void v3Params(struct sift_parameters* p) {
    p->n_oct = 1;
    p->n_spo = 1;
    p->delta_min=1;
    p->sigma_in=1.1;
    p->sigma_min=1.1;
}

/** @brief Extracts oriented keypoints (without description
 *
 *
 */
struct sift_keypoint_std* my_sift_compute_features(struct sift_parameters* p, const float* x, int w, int h, int *n, struct sift_keypoints** outKeypoints)
{   
    /** Memory dynamic allocation */
    // WARNING 6 lists of keypoints containing intermediary states of the algorithm
    struct sift_keypoints **kk = xmalloc(6*sizeof(struct sift_keypoints*));
    for(int i = 0; i < 6; i++){
        kk[i] = sift_malloc_keypoints();
    }
    // WARNING 4 scalespace structures
    struct sift_scalespace **ss = xmalloc(4*sizeof(struct sift_scalespace*));

    /** Algorithm */
    struct sift_keypoints* keys = sift_anatomy(x, w, h, p, ss, kk);

    /* Copy to a list of keypoints */
    *n = keys->size;
    struct sift_keypoint_std* k = xmalloc((*n)*sizeof(*k));
    for(int i = 0; i < keys->size; i++){
        k[i].x = keys->list[i]->x;
        k[i].y = keys->list[i]->y;
        k[i].scale = keys->list[i]->sigma;
        k[i].orientation = keys->list[i]->theta;
        for(int j = 0; j < 128; j++){
            k[i].descriptor[j] = keys->list[i]->descr[j];
        }
    }

    /* memory deallocation */
    //xfree(p);
    *outKeypoints = keys; //sift_free_keypoints(keys);
    for(int i = 0; i < 6; i++){
        sift_free_keypoints(kk[i]);
    }
    xfree(kk);
    for(int i = 0; i < 4; i++){
        sift_free_scalespace(ss[i]);
    }
    xfree(ss);

    return k;
}

int currentVersion = 1001; //1000;
void my_fprintf_parameters(FILE* file, struct sift_parameters* params) {
    // struct sift_parameters
    // {
    //     int n_oct, n_spo, n_hist, n_bins, n_ori, itermax;
    //     float sigma_min, delta_min, sigma_in, C_DoG, C_edge, lambda_ori, t, lambda_descr;
    // };

    fprintf(file, "%d\n%d %d %d %d %d %d " // int n_oct, n_spo, n_hist, n_bins, n_ori, itermax;
            // Note for the below: "The %a formatting specifier is new in C99. It prints the floating-point number in hexadecimal form." ( https://stackoverflow.com/questions/4826842/the-format-specifier-a-for-printf-in-c )
            "%a %a %a %a %a %a %a %a" // float sigma_min, delta_min, sigma_in, C_DoG, C_edge, lambda_ori, t, lambda_descr;
            "\n", currentVersion, params->n_oct, params->n_spo, params->n_hist, params->n_bins, params->n_ori, params->itermax,
            params->sigma_min, params->delta_min, params->sigma_in, params->C_DoG, params->C_edge, params->lambda_ori, params->t, params->lambda_descr);
}
void my_fprintf_matching_parameters(FILE* file,
                                    int meth_flag,
                                    float thresh) {
    fprintf(file, "%d %a\n", meth_flag, thresh);
}

void my_sift_write_to_file(const char *filename, const struct sift_keypoints *keypoints, struct sift_parameters* params, int n)
{
    FILE* file = fopen(filename,"w");
    if ( !file) {
        perror("");
        fatal_error("File \"%s\" could not be opened.", filename);
    }
    my_fprintf_parameters(file, params);
    fprintf_keypoints(file, keypoints, 2);
    fclose(file);
}

void my_sift_load_parameters(FILE* stream, char* buffer, int buffer_size, struct sift_parameters* outParams, int* outVersion) {
    struct sift_parameters* params = outParams;
    if(fgets(buffer, buffer_size, stream) != NULL){
        // read version
        sscanf(buffer, "%d\n", outVersion);
        // check version
        if (*outVersion > currentVersion || *outVersion < 1000 /*<--first version*/) {
            fatal_error("Keypoints or matching file version too large (unsupported version)");
        }
    }
    
    if(fgets(buffer, buffer_size, stream) != NULL){
        // read params
        sscanf(buffer,"%d %d %d %d %d %d "
               "%a %a %a %a %a %a %a %a"
               "\n", &params->n_oct, &params->n_spo, &params->n_hist, &params->n_bins, &params->n_ori, &params->itermax,
               &params->sigma_min, &params->delta_min, &params->sigma_in, &params->C_DoG, &params->C_edge, &params->lambda_ori, &params->t, &params->lambda_descr);
    }
}

// Need to free return value with free() and need to free *outKeypoints with sift_free_keypoints().
// On error, returns NULL.
struct sift_keypoint_std* my_sift_read_from_file(const char *filename, int *n, struct sift_keypoints** outKeypoints)
{
    FILE* stream = fopen(filename,"r");
    if ( !stream) {
        return NULL;
    }

    size_t buffer_size = 1024 * 1024;  // 1MB buffer for long lines.
    char* buffer = xmalloc(buffer_size);
    
    // Load parameters
    struct sift_parameters* params = sift_assign_default_parameters();
    int version;
    my_sift_load_parameters(stream, buffer, buffer_size, params, &version);

    // Load keypoints
    struct sift_keypoints* keys = sift_malloc_keypoints();
    my_sift_read_keypoints(keys, stream, buffer, buffer_size, params->n_hist, params->n_ori, params->n_bins, 2);

    // translating into a flat list
    int l = params->n_hist * params->n_hist * params->n_ori;
    *n = keys->size;
    struct sift_keypoint_std* k = xmalloc((*n)*sizeof(*k));
    for(int i = 0; i < keys->size; i++){
        k[i].x = keys->list[i]->x;
        k[i].y = keys->list[i]->y;
        k[i].scale = keys->list[i]->sigma;
        k[i].orientation = keys->list[i]->theta;

        for(int j = 0; j < l; j++){
            k[i].descriptor[j] = keys->list[i]->descr[j];
        }
    }
    
    //sift_free_keypoints(keys);
    *outKeypoints = keys;
    *n = keys->size;
    xfree(buffer);
    fclose(stream);
    return k;
}

// Returns 1 on failure
int my_sift_read_matches_from_file(const char *filename,
                                                         // Output:
                                                         const struct sift_keypoints *k1,
                                                         const struct sift_keypoints *k2A,
                                                         const struct sift_keypoints *k2B,
                                   int* meth_flag,
                                   float* thresh)
{
    FILE* stream = fopen(filename,"r");
    if ( !stream) {
        return 1;
    }

    size_t buffer_size = 1024 * 1024;  // 1MB buffer for long lines.
    char* buffer = xmalloc(buffer_size);
    
    // Load parameters
    struct sift_parameters* params = sift_assign_default_parameters();
    int version;
    my_sift_load_parameters(stream, buffer, buffer_size, params, &version);
    
    // Load extra params (>=v1001 only)
    if (version >= 1001) {
        if(fgets(buffer, buffer_size, stream) != NULL){
            sscanf(buffer, "%d %a\n", meth_flag, thresh);
        }
    }
    else {
        // Defaults are used
        *meth_flag = 1;
        *thresh = 0.6;
    }

    // Load keypoints
    my_sift_read_matches(k1, k2A, k2B, stream, buffer, buffer_size, params->n_hist, params->n_ori, params->n_bins, 2);
    
    xfree(buffer);
    fclose(stream);
    return 0;
}


/** @brief read list of oriented keypoints from txt file
 *
 * @param keys    =  output list of keypoints
 * @param name    =  input filename
 *
 * @param n_hist  the descriptor has (n_hist*n_hist) weighted coefficients.
 * @param n_ori   the descriptor has (n_hist*n_hist*n_ori) coefficients
 * @param n_bins  the size of the orientation histogram
 *
 */
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
                         int flag)
{
    while(fgets(buffer, buffer_size, stream) != NULL){ // Note: "A newline character makes fgets stop reading" ( https://www.cplusplus.com/reference/cstdio/fgets/ )
        int pos = 0;
        int read = 0;
        my_sift_read_one_keypoint(keys, &pos, &read, buffer, buffer_size, n_hist, n_ori, n_bins, flag);
    }
}

void my_sift_read_one_keypoint(struct sift_keypoints* keys,
                               int* pos,
                               int* read,
                               char* buffer,
                               size_t buffer_size,
                               int n_hist,
                               int n_ori,
                               int n_bins,
                               int flag) {
    struct keypoint* key = sift_malloc_keypoint(n_ori, n_hist, n_bins);
    // read coordinates
    sscanf(buffer + *pos,"%f  %f  %f  %f %n", &key->x
                                         , &key->y
                                         , &key->sigma
                                         , &key->theta
                                         , read);
    *pos += *read;
    if (flag > 0){
        // read descriptor
        for(int i = 0; i < n_hist*n_hist*n_ori; i++){
            sscanf(buffer + *pos, "%f %n",&(key->descr[i]),read);
            *pos += *read;
        }
    }
    if (flag > 1){
        // read orientation histogram
        for(int i=0;i<n_bins;i++){
            sscanf(buffer + *pos, "%f %n",&(key->orihist[i]),read);
            *pos += *read;
        }
    }
    sift_add_keypoint_to_list(key,keys);
}

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
                         int flag)
{
    while(fgets(buffer, buffer_size, stream) != NULL){
        int pos = 0;
        int read = 0;
        
        // Read a keypoint into each keypoints list
        my_sift_read_one_keypoint(k1, &pos, &read, buffer, buffer_size, n_hist, n_ori, n_bins, flag);
        my_sift_read_one_keypoint(k2A, &pos, &read, buffer, buffer_size, n_hist, n_ori, n_bins, flag);
        my_sift_read_one_keypoint(k2B, &pos, &read, buffer, buffer_size, n_hist, n_ori, n_bins, flag);
    }
}

void my_sift_write_matches_to_file(const char *filename,
                                   const struct sift_keypoints *k1,
                                   const struct sift_keypoints *k2A,
                                   const struct sift_keypoints *k2B,
                                   struct sift_parameters* params,
                                   int meth_flag,
                                   float thresh)
{
    FILE* file = fopen(filename,"w");
    if ( !file) {
        perror("");
        fatal_error("File \"%s\" could not be opened.", filename);
    }
    my_fprintf_parameters(file, params);
    my_fprintf_matching_parameters(file, meth_flag, thresh);
    my_save_pairs_extra(file, k1, k2A, k2B);
    fclose(file);
}

void my_save_pairs_extra(FILE* stream,
                      const struct sift_keypoints *k1,
                      const struct sift_keypoints *k2A,
                      const struct sift_keypoints *k2B)
{
    if (k1->size > 0){

        int n_hist = k1->list[0]->n_hist;
        int n_ori = k1->list[0]->n_ori;
        int dim = n_hist*n_hist*n_ori;
        int n_bins  = k1->list[0]->n_bins;
        int n = k1->size;
        for(int i = 0; i < n; i++){
            fprintf_one_keypoint(stream, k1->list[i], dim, n_bins, 2);
            fprintf_one_keypoint(stream, k2A->list[i], dim, n_bins, 2);
            fprintf_one_keypoint(stream, k2B->list[i], dim, n_bins, 2);
            fprintf(stream, "\n");
        }
    }
}
