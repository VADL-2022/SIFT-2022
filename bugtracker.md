### Severe issues

1. Deadlocks with new writePtr-checking matching code introduced about a week before 3-6-2022. Debug using `--debug-mutex-deadlocks` and `--debug-mutex-milliseconds`.

### Issues

1. >1024 null images causes ```if ((i == 0 && processedImageQueue.count == 0) // Edge case for first image
        || (processedImageQueue.writePtr == (i) % processedImageQueueBufferSize) // If we're writing right after the last element, can enqueue our sequentially ordered image
        ) {``` to fail
2. matcher needs to print when impossible happens, indicating something is wrong: when the i it is looking at in processedImageQueue peeks is greater than the previousI+1 (basically when the i incremented more than once between images in the queue). this indicates something is very wrong since an image of lower index than the current one in the queue was skipped (or comes later) in the processedImageQueue.

### Low priority

1. printf needs to be wrapped in out_guard()'s since it can print while out_guard() is held in the middle of some cout's on a different thread.
2. Stops early when running with `--finish-rest-always` in, for example, `make -j8 sift_exe_debug_commandLine && ./sift_exe_debug_commandLine --video-file-data-source --video-file-data-source-path /Volumes/MyTestVolume/Projects/DataRocket/files_sift1_videosTrimmedOnly_fullscale1/2022-02-19_11_31_58_CST/output.mp4 --main-mission --sift-video-output --finish-rest-always --skip-image-indices 2 3 --skip-image-indices 4 6 --skip-image-indices 7 8 --skip-image-indices 14 15 --skip-image-indices 18 21 --skip-image-indices 22 24 --skip-image-indices 25 29 --skip-image-indices 32 33 --skip-image-indices 35 37 --skip-image-indices 39 41 --skip-image-indices 42 43 --skip-image-indices 44 46 --skip-image-indices 47 50 --skip-image-indices 51 55 --skip-image-indices 56 57 --skip-image-indices 58 59 --skip-image-indices 60 61 --skip-image-indices 64 66 --skip-image-indices 67 70 --skip-image-indices 71 74 --skip-image-indices 75 80 --skip-image-indices 81 82 --skip-image-indices 83 85 --skip-image-indices 86 88 --skip-image-indices 89 92 --skip-image-indices 94 95 --skip-image-indices 96 97 --skip-image-indices 99 102 --skip-image-indices 103 105 --skip-image-indices 106 111 --skip-image-indices 112 117 --skip-image-indices 118 122 --skip-image-indices 123 125 --skip-image-indices 128 130 --skip-image-indices 132 136 --skip-image-indices 139 140 --skip-image-indices 141 142 --debug-mutex-deadlocks`

### Possible issues

1. Possible --sleep-before-running 7000 issue for pipe: Try getting `Forking with bash command: XAUTHORITY=/home/pi/.Xauthority ./sift_exe_release_commandLine  --no-preview-window --main-mission --sift-params -C_edge 2 -delta_min 0.6 --sleep-before-running 7000 --subscale-driver-fd 13` to happen on a sift pi from `subscale_exe_release`. Maybe we need to make it grab from pipe sooner than the --sleep-before-running 7000?

### Debugging (3rd to lowest priority, only affects working on SIFT but not in field)

1. --skip-image-indices doesn't work in --main-mission, only --main-mission-interactive
2. --skip-image-indices may not actually skip the last index (i.e. if given  --skip-image-indices 6 10 then it may not skip image index 9)

### Hacks that essentially can always work (2nd to lowest priority)

1. [nvm, problem was canvasesReadyQueue instead because its size is 16 and having to compete locks with processedImageQueue since main needs canvasesReadyQueue] processedImageQueue buffer size used to be 16 but now it is hacked to be the highest number possible during 50 seconds or so. The bug is that matcher thread (enqueuing processed image) and main thread (enqueuing processed image) wait forever on a lock in `pthread_cond_wait` for some reason, only when count == BUFFER_SIZE in Queue.hpp

71 seconds = last year descent time

### Lowest priority bugs since they don't affect real usage

1. Python segfaults after every sigint IF you stop it too soon after a py run file with a sigint
