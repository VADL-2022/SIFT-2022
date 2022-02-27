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
