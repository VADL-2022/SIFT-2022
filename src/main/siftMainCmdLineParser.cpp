//
//  siftMainCmdLineParser.cpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "siftMainCmdLineParser.hpp"
#include "siftMainCmdConfig.hpp"

#include <string.h>
#include <thread>
#include "../DataSource.hpp"

int driverInput_fd = -1; // File descriptor for reading from the driver program, if any (else it is -1)
int driverInput_fd_fcntl_flags = 0;
FILE* driverInput_file = NULL;

#ifdef USE_COMMAND_LINE_ARGS
// Returns 0 on success, else returns the exit code for something like int main().
int parseCommandLineArgs(int argc, char** argv,
                          //Outputs:
                          SIFTState& s, SIFTParams& p, size_t& skip, std::unique_ptr<DataSourceBase>& src) {
#ifdef SIFTAnatomy_
    p.params = sift_assign_default_parameters();
#endif
    
    // Set the default "skip"
    skip = 0;//120;//60;//100;//38;//0;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--folder-data-source") == 0) { // Get from folder (path is specified with --folder-data-source-path, otherwise the default is used) instead of camera
            src = std::make_unique<FolderDataSource>(argc, argv, skip); // Read folder determined by command-line arguments
            cfg.folderDataSource = true;
        }
        if (strcmp(argv[i], "--video-file-data-source") == 0) { // Get from video file (path is specified with --video-file-data-source-path, otherwise the default is used) instead of camera
            src = std::make_unique<VideoFileDataSource>(argc, argv); // Read file determined by command-line arguments
            cfg.videoFileDataSource = true;
        }
        else if (strcmp(argv[i], "--camera-test-only") == 0) {
            cfg.cameraTestOnly = true;
        }
        else if (strcmp(argv[i], "--image-capture-only") == 0) { // For not running SIFT
            cfg.imageCaptureOnly = true;
        }
        else if (strcmp(argv[i], "--image-file-output") == 0) { // [mainInteractive() only] Outputs to video, usually also instead of preview window
            cfg.imageFileOutput = true;
        }
        else if (strcmp(argv[i], "--sift-video-output") == 0) { // Outputs frames with SIFT keypoints, etc. rendered to video file
            cfg.siftVideoOutput = true;
        }
        else if (i+2 < argc && strcmp(argv[i], "--skip-image-indices") == 0) { // Ignore images with this index. Usage: `--skip-image-indices 1 2` to skip images 1 and 2 (0-based indices, end index is exclusive)
            cfg.skipImageIndices.push_back(std::make_pair(std::stoi(argv[i+1]), std::stoi(argv[i+2])));
            i += 2;
        }
        else if (strcmp(argv[i], "--save-first-image") == 0) { // Save the first image SIFT encounters. Counts as a flush towards the start of SIFT.
            cfg.saveFirstImage = true;
        }
        else if (strcmp(argv[i], "--no-preview-window") == 0) { // Explicitly disable preview window
            cfg.noPreviewWindow = true;
        }
        else if (i+1 < argc && strcmp(argv[i], "--save-every-n-seconds") == 0) { // [mainMission() only] Save video and transformation matrices every n (given) seconds. Set to -1 to disable flushing until SIFT exits.
            cfg.flushVideoOutputEveryNSeconds = std::stoi(argv[i+1]);
            i++;
        }
        else if (strcmp(argv[i], "--main-mission") == 0) {
            cfg.mainMission = true;
        }
        else if (strcmp(argv[i], "--main-mission-interactive") == 0) { // Is --main-mission, but waits forever after a frame is shown in the preview window
            cfg.mainMission = true;
            cfg.waitKeyForever = true;
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            cfg.verbose = true;
        }
        else if (i+1 < argc && strcmp(argv[i], "--sleep-before-running") == 0) {
            long long time = std::stoll(argv[i+1]); // Time in milliseconds
            // Special value: -1 means sleep default time
            if (time == -1) {
                time = 30 * 1000;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
            i++;
        }
        else if (i+1 < argc && strcmp(argv[i], "--subscale-driver-fd") == 0) { // For grabbing IMU data, SIFT requires a separate driver program writing to the file descriptor given.
            driverInput_fd = std::stoi(argv[i+1]);
            printf("driverInput_fd set to: %d\n", driverInput_fd);
            i++;
        }
        else if (strcmp(argv[i], "--debug-no-std-terminate") == 0) { // Disables the SIFT unhandled exception handler for C++ exceptions. Use for lldb purposes.
            cfg.useSetTerminate = false;
        }
#ifdef SIFTAnatomy_
        else if (i+1 < argc && strcmp(argv[i], "--sift-params") == 0) {
            for (int j = i+1; j < argc; j++) {
#define LOG_PARAM(x) std::cout << "Set " #x << " to " << p.params->x << std::endl
                if (j+1 < argc && strcmp(argv[j], "-n_oct") == 0) {
                    p.params->n_oct = std::stoi(argv[j+1]);
                    LOG_PARAM(n_oct);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_spo") == 0) {
                    p.params->n_spo = std::stoi(argv[j+1]);
                    LOG_PARAM(n_spo);
                }
                else if (j+1 < argc && strcmp(argv[j], "-sigma_min") == 0) {
                    p.params->sigma_min = std::stof(argv[j+1]);
                    LOG_PARAM(sigma_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-delta_min") == 0) {
                    p.params->delta_min = std::stof(argv[j+1]);
                    LOG_PARAM(delta_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-sigma_in") == 0) {
                    p.params->sigma_in = std::stof(argv[j+1]);
                    LOG_PARAM(sigma_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-C_DoG") == 0) {
                    p.params->C_DoG = std::stof(argv[j+1]);
                    LOG_PARAM(sigma_min);
                }
                else if (j+1 < argc && strcmp(argv[j], "-C_edge") == 0) {
                    p.params->C_edge = std::stof(argv[j+1]);
                    LOG_PARAM(C_edge);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_bins") == 0) {
                    p.params->n_bins = std::stoi(argv[j+1]);
                    LOG_PARAM(n_bins);
                }
                else if (j+1 < argc && strcmp(argv[j], "-lambda_ori") == 0) {
                    p.params->lambda_ori = std::stof(argv[j+1]);
                    LOG_PARAM(lambda_ori);
                }
                else if (j+1 < argc && strcmp(argv[j], "-t") == 0) {
                    p.params->t = std::stof(argv[j+1]);
                    LOG_PARAM(t);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_hist") == 0) {
                    p.params->n_hist = std::stoi(argv[j+1]);
                    LOG_PARAM(n_hist);
                }
                else if (j+1 < argc && strcmp(argv[j], "-n_ori") == 0) {
                    p.params->n_ori = std::stoi(argv[j+1]);
                    LOG_PARAM(n_ori);
                }
                else if (j+1 < argc && strcmp(argv[j], "-lambda_descr") == 0) {
                    p.params->lambda_descr = std::stof(argv[j+1]);
                    LOG_PARAM(lambda_descr);
                }
                else if (j+1 < argc && strcmp(argv[j], "-itermax") == 0) {
                    p.params->itermax = std::stoi(argv[j+1]);
                    LOG_PARAM(itermax);
                }
                else {
                    // Continue with these parameters as the next arguments
                    i = j-1; // It will increment next iteration, so -1.
                    break;
                }
                j++; // Skip the number we parsed
            }
        }
        else if (i+1 < argc && strcmp(argv[i], "--sift-params-func") == 0) {
            if (SIFTParams::call_params_function(argv[i+1], p.params) < 0) {
                printf("Invalid params function name given: %s. Value options are:", argv[i+1]);
                SIFTParams::print_params_functions();
                printf(". Exiting.\n");
                exit(1);
            }
            else {
                printf("Using params function %s\n", argv[i+1]);
            }
            i++;
        }
#endif
        else {
            // Whitelist: allow these to go through since they can be passed to DataSources, etc. //
            if (strcmp(argv[i], "--video-file-data-source-path") == 0) {
                i++; // Also go past the extra argument for this argument
                continue;
            }
            else if (strcmp(argv[i], "--read-backwards") == 0) {
                continue;
            }
            else if (strcmp(argv[i], "--crop-for-fisheye-camera") == 0) {
                continue;
            }
            else if (strcmp(argv[i], "--fps") == 0) {
                i++; // Also go past the extra argument for this argument
                continue;
            }
            // //
            
            printf("Unrecognized command-line argument given: %s", argv[i]);
            printf(" (command line was:\n");
            for (int i = 0; i < argc; i++) {
                printf("\t%s\n", argv[i]);
            }
            printf(")\n");
            puts("Exiting.");
            return 1;
        }
    }
    
    return 0;
}
#endif
