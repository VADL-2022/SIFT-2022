# https://learnopencv.com/read-write-and-display-a-video-using-opencv-cpp-python/

import cv2
import numpy as np
from datetime import datetime
import threading
import os
import stat
import timeit
from queue import *
import time

class AtomicInt:
    def __init__(self, initial=0):
        self.value=initial
        self._lock = threading.Lock()

    def incrementAndThenGet(self, num=1):
        with self._lock:
            self.value+=num
            return self.value

    def decrementAndThenGet(self, num=1):
        with self._lock:
            self.value-=num
            return self.value
    
    def set(self, num):
        with self._lock:
            self.value = num
        
    def get(self):
        with self._lock:
            return self.value

format=cv2.VideoWriter_fourcc('X', 'V', 'I', 'D')
#format=cv2.VideoWriter_fourcc('a', 'v', 'c', '1')

mainThreadShouldFlush = AtomicInt(0) # 0 if no longer using temporarily shared VideoWriter (between queue and main thread), 1 if so. Also known as, 0 if main thread needs to flush the video in case of Ctrl-C/other exception in main thread *before* main thread can set out = None so it knows not to flush while a dispatchQueue function was already enqueued to flush.

dispatchQueue = Queue()
def dispatchQueueThreadFunc(name, shouldStop):
    # name = nameAndShouldStop[0]
    # shouldStop = nameAndShouldStop[1]
    print("videoCapture: Thread %s: starting" % name)
    # Based on bottom of page at https://docs.python.org/2/library/queue.html#module-Queue
    while shouldStop.get() == 0:
        try:
            item = dispatchQueue.get(timeout=0.1) # "If timeout is a positive number, it blocks at most timeout seconds and raises the Empty exception if no item was available within that time. Otherwise (block is false), return an item if one is immediately available, else raise the Empty exception (timeout is ignored in that case)." ( https://docs.python.org/3/library/queue.html#queue.Queue.get ) + https://stackoverflow.com/questions/46899021/how-to-abort-threads-that-pull-items-from-a-queue-using-ctrlc-in-python : "Queue.get() will ignore SIGINT unless a timeout is set. That's documented here: https://bugs.python.org/issue1360."
        except Empty:
            continue
        if mainThreadShouldFlush.get() == 0:
            item()
        else:
            print("videoCapture: Thread %s: not flushing due to mainThreadShouldFlush" % name)
            # Wait till it changes, then flush. If it never changes and main says we should stop, then exit.
            while mainThreadShouldFlush.get() != 0:
                time.sleep(0.2)
                if shouldStop.get() == 1:
                    print("videoCapture: Thread %s: finishing without flush" % name)
                    return
            item()
        dispatchQueue.task_done()
    print("videoCapture: Thread %s: finishing" % name)
num_worker_threads = 1 #<--due to use of mainThreadShouldFlush, we need to use 1 here (else worker threads might not flush all their videos).. could try changing mainThreadShouldFlush to be something better if needed   #2

def run(shouldStop # AtomicInt
        , verbose = False
        , onFrame = None # handler function that accepts an image
        , fps = 30
        , frame_width=640
        , frame_height=480
        , onSetVideoCapture = None # handler for when video capture object changes
        , outputFolderPath = None
        , capOrig=None
        , noWriter=False
        ):
    global mainThreadShouldFlush
    global dispatchQueue
    mainThreadShouldFlush = AtomicInt(0)
    dispatchQueue = Queue()

    if not noWriter:
        for i in range(num_worker_threads):
            t = threading.Thread(target=dispatchQueueThreadFunc, args=('dispatchQueueThreadFunc',shouldStop))
            t.daemon = True
            t.start()
     
    now = datetime.now() # current date and time
    lastFlush=now
    
    # Create a VideoCapture object
    cap = cv2.VideoCapture(0) if capOrig is None else capOrig
    #print("Total frames in cap:",cap.get(cv2.CAP_PROP_FRAME_COUNT))
    cap.set(3, frame_width)
    cap.set(4, frame_height)

    if onSetVideoCapture is not None:
        onSetVideoCapture(cap)

    # Check if camera opened successfully
    if (cap.isOpened() == False): 
      print("Unable to read camera feed")

    # Default resolutions of the frame are obtained.The default resolutions are system dependent.
    # We convert the resolutions from float to integer.
    frame_width_ = int(cap.get(3))
    frame_height_ = int(cap.get(4))

    # Define the codec and create VideoWriter object.The output is stored in 'outpy.avi' file.
    date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
    print("Requested width, height:", frame_width,frame_height)
    print("Current width,height:",frame_width_,frame_height_)
    if outputFolderPath is None:
        o1=now.strftime("%Y-%m-%d_%H_%M_%S_%Z")
        outputFolderPath=os.path.join('.', 'dataOutput', o1)
    p=os.path.join(outputFolderPath,'outpy' + date_time + '.mp4')
    try:
      os.mkdir(os.path.dirname(p), mode=stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
    except FileExistsError:
      pass
    if not noWriter:
        out = cv2.VideoWriter(p, format, fps, (frame_width,frame_height))
        outMainThreadShouldFlush = out
    else:
        out = None
        outMainThreadShouldFlush = None

    try:
        while(shouldStop.get() == 0):
          ret, frame = cap.read()

          if ret == True: 

              # Write the frame into the file 'output.avi'
              if verbose:
                  print("out.write(frame) took", timeit.timeit(lambda: out.write(frame), number=1), "seconds")
              elif not noWriter:
                  out.write(frame)
              if onFrame is not None:
                  onFrame(frame)

              # Display the resulting frame    
              #cv2.imshow('frame',frame)

              # Press Q on keyboard to stop recording
              #if cv2.waitKey(1) & 0xFF == ord('q'):
              #  break

          # Break the loop
          else:
            break

          now = datetime.now()
          duration=now-lastFlush
          if duration.total_seconds() >= 2 and not noWriter:
              lastFlush = now
              # Flush video
              out2=out
              def flushFn():
                  print("out.release() took", timeit.timeit(lambda: out2.release(), number=1), "seconds")
                  print("Flushed the video")
              mainThreadShouldFlush.incrementAndThenGet() # Mark main thread as having to flush on behalf of the queue since if an exception occurs anywhere between this statement and the corresponding decrementAndThenGet(), then we want the queue thread to realize that main thread will handle the queued item on its behalf.
              print("Enqueuing flush") # If an exception occurs after this, <same as below comment>
              dispatchQueue.put(flushFn) # If an exception occurs after this, <same as below comment>
              outMainThreadShouldFlush = out # If an exception occurs after this, out is not None, therefore a `"main thread final flush`[...] will occur.
              out = None # If exception occurs after this, we know outMainThreadShouldFlush = the video writer object, and out = None, so the `"main thread flushing unflushed edge case"` should fire.
              mainThreadShouldFlush.decrementAndThenGet() # If exception/preemption occurs after we decrement this, we know `out` is None so main thread has nothing to flush, and the queue will handle it.
              date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
              p=os.path.join(outputFolderPath,'outpy' + date_time + '.mp4')
              print("Making new VideoWriter at", p)
              out = cv2.VideoWriter(p,format, fps, (frame_width,frame_height))
              print("Made new VideoWriter at", p)
    except KeyboardInterrupt:
        print("Handing keyboardinterrupt")
        pass
    finally:
        # When everything done, release the video capture and video write objects
        cap.release()
        if outMainThreadShouldFlush is not None and out is None and mainThreadShouldFlush.get() == 1:
            print("main thread flushing unflushed edge case")
            out = outMainThreadShouldFlush
        if out is not None:
            print("main thread final flush: out.release() took", timeit.timeit(lambda: out.release(), number=1), "seconds")
        
        shouldStop.set(1) #shouldStop.incrementAndThenGet() # Stop threads in case it wasn't done already
        
        #dispatchQueue.join()       # block until all tasks are done

        # Closes all the frames
        #cv2.destroyAllWindows()

if __name__ == "__main__":
  run(AtomicInt(0))
