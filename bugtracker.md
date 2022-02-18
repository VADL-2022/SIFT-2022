

### Debugging (2nd to lowest priority, only affects working on SIFT but not in field)

1. --skip-image-indices doesn't work in --main-mission, only --main-mission-interactive
2. --skip-image-indices may not actually skip the last index (i.e. if given  --skip-image-indices 6 10 then it may not skip image index 9)

### Hacks that essentially can always work (lowest priority)

1. [nvm, problem was canvasesReadyQueue instead because its size is 16 and having to compete locks with processedImageQueue since main needs canvasesReadyQueue] processedImageQueue buffer size used to be 16 but now it is hacked to be the highest number possible during 50 seconds or so. The bug is that matcher thread (enqueuing processed image) and main thread (enqueuing processed image) wait forever on a lock in `pthread_cond_wait` for some reason, only when count == BUFFER_SIZE in Queue.hpp

71 seconds = last year descent time
