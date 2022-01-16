//
//  Queue.hpp
//  SIFT
//
//  Created by VADL on 11/13/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef threadsafe_circular_buffer_h
#define threadsafe_circular_buffer_h

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

// Buffer code from ex_buff.c //

//// An element of a circular buffer.
//struct elem {
//    char c;
//    int i;
//};

// A thread-safe circular buffer.
template <typename T, size_t BUFFER_SIZE>
struct Queue {
//#define BUFFER_SIZE 16
    T content[BUFFER_SIZE]{}; // value-initialization (calls default ctors) ( https://newbedev.com/default-initialization-of-std-array )
    int readPtr = 0; // Index for next element to be read.
    int writePtr = 0; // Index for next element to be inserted.
    int count = 0; // How many elements are currently used in the buffer.
    pthread_mutex_t mutex; // Mutex to lock the buffer.
    pthread_cond_t condition; // The condition variable that threads can wait on.
    
    // Initializes a buffer.
    Queue() {
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&condition, NULL);
    }
    
    // Adds to the buffer without locking
    template< class... Args >
    void enqueueNoLock(Args&&... args) {
        while( count == BUFFER_SIZE ) // Wait until not full.
        {
           pthread_cond_wait( &condition, &mutex );
        }

        // Place in the new value:
        T* e = &content[writePtr];
        *e = T{std::forward<Args>(args)...};
        //e->~T();
//        new (e) T{std::forward<Args>(args)...}; // Note: `cv::Mat::Mat    (    const Mat &     m    )` only increments refcount of that matrix (copies `m`'s header (or pointer to it?) into `this` and then increments refcount), which is what we want here: https://docs.opencv.org/3.4/d3/d63/classcv_1_1Mat.html#a294eaf8a95d2f9c7be19ff594d06278e

        // Increment the write pointer modulo the BUFFER_SIZE:
        writePtr = (writePtr + 1) % BUFFER_SIZE;

        // Increment the element count:
        count++;

        printf("Enqueue: %p\n", (void*)e);
        fflush(stdout);

        // Wake up all other threads waiting on the condition variable
        pthread_cond_broadcast(&condition);
        // Signal only tells one thread that's waiting, whereas broadcast tells all threads waiting on the condition.
    }

    // Adds to the buffer.
    template< class... Args >
    void enqueue(Args&&... args) {
        pthread_mutex_lock( &mutex );
        enqueueNoLock(std::forward<Args>(args)...);
        // Unlock the mutex:
        pthread_mutex_unlock( &mutex );
    }
    
    // Gets the next element that should be read from (consumed). To consume + release it, call `dequeue()` after this.
    T& front() {
        return content[readPtr];
    }
    
    // Gets the element at the given index relative to the current front of the queue, modulo the BUFFER_SIZE.
    T& get(size_t i) {
        return content[readPtr + i % BUFFER_SIZE];
    }
    
    size_t size() {
        pthread_mutex_lock( &mutex );
        auto ret = count;
        pthread_mutex_unlock(&mutex);
        return ret;
    }
    
    bool empty() {
        pthread_mutex_lock( &mutex );
        bool ret = count == 0;
        pthread_mutex_unlock(&mutex);
        return ret;
    }
    
    void dequeueNoLock(T* output) {
        // remove the next element from the buffer, increment the read pointer (modulo buffer size)
        // and decrement the counter,
        // wake up all other threads waiting on the condition variable, and then unlock the mutex and return.
        T* e = &content[readPtr];
        *output = *e;
        readPtr = (readPtr + 1) % BUFFER_SIZE;
        count--;

        printf("Dequeue: %p with %d left\n", e, count); // Need to print inside
        // the lock! Because print could get preempted (a context switch could happen) *right* between print
        // and fflush, so that the actual printing to the console that we see happens way later, after already
        // context switching!
        fflush(stdout);
    }
        
    // Removes from the buffer.
    void dequeue(T* output) {
        pthread_mutex_lock( &mutex );
        while( count == 0 ) // Wait until not empty.
        {
           pthread_cond_wait( &condition, &mutex );
        }

        dequeueNoLock(output);

        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&mutex);
    }
    
    void peekTwoImagesNoLock(T* output1, T* output2) {
        assert( count > 1 );

        // remove the next element from the buffer, increment the read pointer (modulo buffer size)
        // and decrement the counter,
        // wake up all other threads waiting on the condition variable, and then unlock the mutex and return.
        T* e = &content[readPtr];
        *output1 = *e;
        T* e2 = &content[(readPtr + 1) % BUFFER_SIZE];
        *output2 = *e2;

        printf("Peek on two images: %p and %p with %d left\n", e, e2, count); // Need to print inside
        // the lock! Because print could get preempted (a context switch could happen) *right* between print
        // and fflush, so that the actual printing to the console that we see happens way later, after already
        // context switching!
        fflush(stdout);
    }
    
    void peekThirdImageNoLock(T* output) {
        assert( count > 2 );

        // remove the next element from the buffer, increment the read pointer (modulo buffer size)
        // and decrement the counter,
        // wake up all other threads waiting on the condition variable, and then unlock the mutex and return.
        T* e = &content[(readPtr + 2) % BUFFER_SIZE];
        *output = *e;

        printf("Peek on third image: %p with %d left\n", e, count); // Need to print inside
        // the lock! Because print could get preempted (a context switch could happen) *right* between print
        // and fflush, so that the actual printing to the console that we see happens way later, after already
        // context switching!
        fflush(stdout);
    }
    
    // Removes one item from the buffer but takes two items from the buffer (the previous one as well).
    void dequeueOnceOnTwoImages(T* output1, T* output2) {
        pthread_mutex_lock( &mutex );
        while( count <= 1 ) // Wait until not empty and not one left.
        {
           pthread_cond_wait( &condition, &mutex );
        }

        // remove the next element from the buffer, increment the read pointer (modulo buffer size)
        // and decrement the counter,
        // wake up all other threads waiting on the condition variable, and then unlock the mutex and return.
        T* e = &content[readPtr];
        *output1 = *e;
        T* e2 = &content[(readPtr + 1) % BUFFER_SIZE];
        *output2 = *e2;
        readPtr = (readPtr + 1) % BUFFER_SIZE;
        count--;

        printf("Dequeue once on two images: %p and %p with %d left\n", e, e2, count); // Need to print inside
        // the lock! Because print could get preempted (a context switch could happen) *right* between print
        // and fflush, so that the actual printing to the console that we see happens way later, after already
        // context switching!
        fflush(stdout);

        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&mutex);
    }
    
    void dequeueTwiceNoLock() {
        // remove the next element from the buffer, increment the read pointer (modulo buffer size)
        // and decrement the counter,
        // wake up all other threads waiting on the condition variable, and then unlock the mutex and return.
        readPtr = (readPtr + 2) % BUFFER_SIZE;
        count-=2;

        printf("Dequeue twice: %d left\n", count); // Need to print inside
        // the lock! Because print could get preempted (a context switch could happen) *right* between print
        // and fflush, so that the actual printing to the console that we see happens way later, after already
        // context switching!
        fflush(stdout);
    }
    
    // Dequeue with no output
    void dequeueTwice() {
        pthread_mutex_lock( &mutex );
        while( count <= 1 ) // Wait until not empty and not one left.
        {
           pthread_cond_wait( &condition, &mutex );
        }

        dequeueTwiceNoLock();

        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&mutex);
    }
};

// //


#endif /* threadsafe_circular_buffer_h */
