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

void my_fprintf_parameters(FILE* file, struct sift_parameters* params) {
    // struct sift_parameters
    // {
    //     int n_oct, n_spo, n_hist, n_bins, n_ori, itermax;
    //     float sigma_min, delta_min, sigma_in, C_DoG, C_edge, lambda_ori, t, lambda_descr;
    // };

    fprintf(file, "%d %d %d %d %d %d" // int n_oct, n_spo, n_hist, n_bins, n_ori, itermax;
            // Note for the below: "The %a formatting specifier is new in C99. It prints the floating-point number in hexadecimal form." ( https://stackoverflow.com/questions/4826842/the-format-specifier-a-for-printf-in-c )
            "%a %a %a %a %a %a %a %a" // float sigma_min, delta_min, sigma_in, C_DoG, C_edge, lambda_ori, t, lambda_descr;
            "\n", params->n_oct, params->n_spo, params->n_hist, params->n_bins, params->n_ori, params->itermax,
            params->sigma_min, params->delta_min, params->sigma_in, params->C_DoG, params->C_edge, params->lambda_ori, params->t, params->lambda_descr);
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

void my_sift_load_parameters(FILE* stream, char* buffer, int buffer_size, struct sift_parameters* outParams) {
    struct sift_parameters* params = outParams;
    if(fgets(buffer, buffer_size, stream) != NULL){
        // read coordinates
        sscanf(buffer,"%d %d %d %d %d %d"
               "%a %a %a %a %a %a %a %a"
               "\n", &params->n_oct, &params->n_spo, &params->n_hist, &params->n_bins, &params->n_ori, &params->itermax,
               &params->sigma_min, &params->delta_min, &params->sigma_in, &params->C_DoG, &params->C_edge, &params->lambda_ori, &params->t, &params->lambda_descr);
    }
}

// Need to free return value with free() and need to free *outKeypoints with sift_free_keypoints()
struct sift_keypoint_std* my_sift_read_from_file(const char *filename, int *n, struct sift_keypoints** outKeypoints)
{
    FILE* stream = fopen(filename,"r");
    if ( !stream) {
        perror("");
        fatal_error("File \"%s\" could not be opened.", filename);
    }

    size_t buffer_size = 1024 * 1024;  // 1MB buffer for long lines.
    char* buffer = xmalloc(buffer_size);
    
    // Load parameters
    struct sift_parameters* params = sift_assign_default_parameters();
    my_sift_load_parameters(stream, buffer, buffer_size, params);

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
    while(fgets(buffer, buffer_size, stream) != NULL){
        int pos = 0;
        int read = 0;
        struct keypoint* key = sift_malloc_keypoint(n_ori, n_hist, n_bins);
        // read coordinates
        sscanf(buffer+pos,"%f  %f  %f  %f %n", &key->x
                                             , &key->y
                                             , &key->sigma
                                             , &key->theta
                                             , &read);
        pos+=read;
        if (flag > 0){
            // read descriptor
            for(int i = 0; i < n_hist*n_hist*n_ori; i++){
                sscanf(buffer+pos, "%f %n",&(key->descr[i]),&read);
                pos +=read;
            }
        }
        if (flag > 1){
            // read orientation histogram
            for(int i=0;i<n_bins;i++){
                sscanf(buffer+pos, "%f %n",&(key->orihist[i]),&read);
                pos += read;
            }
        }
        sift_add_keypoint_to_list(key,keys);
    }
}
