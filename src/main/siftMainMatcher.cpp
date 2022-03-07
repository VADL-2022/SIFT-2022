//
//  siftMainMatcher.cpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "siftMainMatcher.hpp"
#include "siftMainCmdConfig.hpp"
#include "siftMainSIFT.hpp"
#include "siftMainSignals.hpp"
#include "../siftMain.hpp"

#include "../DataSource.hpp"
#include "../DataOutput.hpp"
#include "../utils.hpp"
#include "../pthread_mutex_lock_.h"

#ifdef USE_COMMAND_LINE_ARGS
DataSourceBase* g_src;
#endif

void onMatcherFinishedMatching(ProcessedImage<SIFT_T>& img2, bool showInPreviewWindow, bool useIdentityMatrix = false) {
    { out_guard();
        std::cout << "Matcher thread: finished matching" << std::endl; }
    
    // Accumulate homography
    if (!useIdentityMatrix) {
        cv::Mat M = img2.transformation.inv();
        cv::Ptr<cv::Formatted> str;
        matrixToString(M, str);
        { out_guard();
            std::cout << "Accumulating new matrix: " << str << std::endl; }
        lastImageToFirstImageTransformationMutex.lock();
        if (lastImageToFirstImageTransformation.empty()) {
            lastImageToFirstImageTransformation = M;
        }
        else {
            lastImageToFirstImageTransformation *= M;
        }
        lastImageToFirstImageTransformationMutex.unlock();
    }
    
    if (showInPreviewWindow) {
        if (CMD_CONFIG(showPreviewWindow())) {
            #ifdef USE_COMMAND_LINE_ARGS
            //assert(!img2.canvas.empty());
            if (img2.canvas.empty()) {
                std::cout << "Empty canvas image, avoiding showing it on canvasesReadyQueue\n";
            }
            else {
                canvasesReadyQueue.enqueue(img2);
            }
            #endif
        }
    }
}
// Assumes processedImageQueue.mutex is locked already.
void matcherWaitForNonPlaceHolderImageNoLockOnlyWait(bool seekInsteadOfDequeue, const int& currentIndex, const char* extraDescription="") {
    while( processedImageQueue.count < 1 + (seekInsteadOfDequeue ? currentIndex : 0) ) // Wait until more images relative to the starting position (`currentIndex`) are in the queue. (Usually `currentCount` is 0 but if it is >= 1 then it is because we're peeking further.)
    {
//        if (stoppedMain()) {
//            pthread_mutex_unlock( &processedImageQueue.mutex );
//            // Stop matcher thread
//            pthread_exit(0);
//        }
        pthread_cond_wait( &processedImageQueue.condition, &processedImageQueue.mutex );
        { out_guard();
            std::cout << "Matcher thread: Unlocking for matcherWaitForImageNoLock 2" << extraDescription << std::endl; }
        pthread_mutex_unlock( &processedImageQueue.mutex );
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        { out_guard();
            std::cout << "Matcher thread: Locking for matcherWaitForImageNoLock 2" << extraDescription << std::endl; }
        pthread_mutex_lock_( &processedImageQueue.mutex );
    }
}
// Assumes processedImageQueue.mutex is locked already.
void matcherWaitForNonPlaceholderImageNoLock(bool seekInsteadOfDequeue, int& currentIndex /*input and output*/) {
    t.reset();
    while (true) {
        matcherWaitForNonPlaceHolderImageNoLockOnlyWait(seekInsteadOfDequeue, currentIndex);

        // Dequeue placeholder/null or non-null images as needed
        ProcessedImage<SIFT_T>& front = seekInsteadOfDequeue ? processedImageQueue.get(currentIndex) : processedImageQueue.front();
        currentIndex++;
        if (front.n < 4 /*"null" image*/) {
            if (seekInsteadOfDequeue) {
                // Peek is done already since we did currentIndex++, nothing to do here.
                { out_guard();
                    std::cout << "Matcher thread: Seeked to null image with index " << currentIndex-1 << std::endl; }
            }
            else {
                { out_guard();
                    std::cout << "Matcher thread: Dequeued null image with index " << currentIndex-1 << std::endl; }
                // Dequeue (till non-null using the loop containing these statements)
                ProcessedImage<SIFT_T> placeholder;
                // Prevents waiting forever if last image we process is null //
//                matcherWaitForNonPlaceHolderImageNoLockOnlyWait(seekInsteadOfDequeue, currentIndex, ": dequeue till non-null");
                // //
                processedImageQueue.dequeueNoLock(&placeholder);
                if (CMD_CONFIG(showPreviewWindow()) && !seekInsteadOfDequeue) {
                    #ifdef USE_COMMAND_LINE_ARGS
                    assert(!placeholder.image.empty());
                    pthread_mutex_unlock( &processedImageQueue.mutex ); // Don't be "greedy"
                    canvasesReadyQueue.enqueueNoLock(placeholder);
                    pthread_mutex_lock_( &processedImageQueue.mutex );
                    #endif
                }
            }
        }
        else {
            { out_guard();
                std::cout << "Matcher thread: Found non-null image with index " << currentIndex-1 << std::endl; }
            // Dequeue and save the image outside this function, since we found it
            break; // `currentIndex` now points to the image we found that is non-null
        }
    }
    t.logElapsed("matcherWaitForNonPlaceholderImageNoLock");
}
int matcherWaitForTwoImages(ProcessedImage<SIFT_T>* img1 /*output*/, ProcessedImage<SIFT_T>* img2 /*output*/) {
    { out_guard();
        std::cout << "Matcher thread: Locking for 2x matcherWaitForImageNoLock" << std::endl; }
    pthread_mutex_lock_( &processedImageQueue.mutex );
    int currentIndex = 0;
    { out_guard();
        std::cout << "matcherWaitForNonPlaceholderImageNoLock: first image" << std::endl; }
    matcherWaitForNonPlaceholderImageNoLock(false, currentIndex); // Dequeue a bunch of null images
    
    // Save the image from `matcherWaitForNonPlaceholderImageNoLock`
    processedImageQueue.dequeueNoLock(img1);
    //*img = processedImageQueue.content[processedImageQueue.readPtr + iMin];
    
    currentIndex = 0;
    std::cout << "matcherWaitForNonPlaceholderImageNoLock: second image\n";
    matcherWaitForNonPlaceholderImageNoLock(false, currentIndex); // Dequeue a bunch of null images
    
    // Save the image from `matcherWaitForNonPlaceholderImageNoLock` but with a peek instead of dequeue since the next matching round requires img2 as its img1
    processedImageQueue.peekNoLock(img2);
    
    pthread_mutex_unlock( &processedImageQueue.mutex );
    { out_guard();
        std::cout << "Matcher thread: Unlocked for 2x matcherWaitForImageNoLock" << std::endl; }
    
    return currentIndex;
}
void* matcherThreadFunc(void* arg) {
    installSignalHandlers();
    
    #ifdef __APPLE__
    // https://stackoverflow.com/questions/2369738/how-to-set-the-name-of-a-thread-in-linux-pthreads
    // "Mac OS X: must be set from within the thread (can't specify thread ID)"
    pthread_setname_np("matcherThread");
    #endif
    
    SIFTState* s = (SIFTState*)arg;
    
    do {
//        OPTICK_THREAD("Worker");
        
        ProcessedImage<SIFT_T> img1, img2;
        int currentIndex = matcherWaitForTwoImages(&img1, &img2);
        
        // Setting current parameters for matching
        img2.applyDefaultMatchingParams();

        // Matching and homography
        t.reset();
        MatchResult enoughDescriptors = sift.findHomography(img1, img2, *s // Writes to img2.transformation
#ifdef USE_COMMAND_LINE_ARGS
                            , g_src, cfg
#endif
                            );
        t.logElapsed("find homography for second image");
        assert(enoughDescriptors != MatchResult::NotEnoughKeypoints); // This should be handled beforehand in the SIFT threads
        // If not enough for second image, we can match with a third.
        if (enoughDescriptors == MatchResult::NotEnoughMatchesForSecondImage) {
            std::cout << "Matcher thread: Not enough descriptors for second image detected. Retrying on third image.\n";
            
            // Usually, SIFT threads can predict this by too low numbers of keypoints. 50 keypoints minimum, raises parameters over time beforehand.
            // Retry matching with the next image that comes in. If this fails, discard the last two images and continue.
            ProcessedImage<SIFT_T> img3;

            { out_guard();
                std::cout << "Matcher thread: Locking for peek third image" << std::endl; }
            pthread_mutex_lock_( &processedImageQueue.mutex );
            matcherWaitForNonPlaceholderImageNoLock(true, currentIndex); // Peek a bunch of null images
            pthread_mutex_unlock( &processedImageQueue.mutex );
            { out_guard();
                std::cout << "Matcher thread: Unlocking for peek third image" << std::endl; }

            t.reset();
            MatchResult enoughDescriptors2 = sift.findHomography(img1, img3, *s // Writes to img3.transformation
            #ifdef USE_COMMAND_LINE_ARGS
                                        , g_src, cfg
            #endif
                                );
            t.logElapsed("find homography for third image");
            
            if (enoughDescriptors2 != MatchResult::Success) {
                std::cout << "Not enough descriptors the second time around\n";
            }
            else if (enoughDescriptors2 != MatchResult::NotEnoughKeypoints) {
                std::cout << "Got enough descriptors the second time around\n";
            }

            onMatcherFinishedMatching(img3, false, enoughDescriptors2 != MatchResult::Success);
            
            // Free memory and mark this as an unused ProcessedImage (optional):
            img1.resetMatching();
            img2.resetMatching();
            img3.resetMatching();
            
            // Regardless of enoughDescriptors2, we still continue with next iteration.
        }
        else if (enoughDescriptors == MatchResult::NotEnoughMatchesForFirstImage) {
            // Nothing we can do with the third image since this first image is the problem, so we skip this image
            // (Use identity matrix for this transformation)
            onMatcherFinishedMatching(img2, true, true);

            // Free memory and mark this as an unused ProcessedImage (optional):
            img1.resetMatching();
            img2.resetMatching();
        }
        else {
            // Save this good match
            onMatcherFinishedMatching(img2, true, false);
            
            // Free memory and mark this as an unused ProcessedImage (optional):
            img1.resetMatching();
            img2.resetMatching();
        }
    } while (!stoppedMain());
    return (void*)0;
}
