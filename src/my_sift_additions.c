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
struct sift_keypoint_std* my_sift_compute_features(const float* x, int w, int h, int *n, struct sift_keypoints** outKeypoints)
{

    /** assign default parameters **/
    struct sift_parameters* p = sift_assign_default_parameters();
    v3Params(p);
    
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
    xfree(p);
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
