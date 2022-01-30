# https://learnopencv.com/read-write-and-display-a-video-using-opencv-cpp-python/

import cv2
import numpy as np
from datetime import datetime
import threading
import os
import stat

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
    
    def get(self):
        with self._lock:
            return self.value

def run(shouldStop # AtomicInt
        ):
    now = datetime.now() # current date and time
    lastFlush=now
    
    # Create a VideoCapture object
    cap = cv2.VideoCapture(0)

    # Check if camera opened successfully
    if (cap.isOpened() == False): 
      print("Unable to read camera feed")

    # Default resolutions of the frame are obtained.The default resolutions are system dependent.
    # We convert the resolutions from float to integer.
    frame_width = int(cap.get(3))
    frame_height = int(cap.get(4))

    # Define the codec and create VideoWriter object.The output is stored in 'outpy.avi' file.
    date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
    fps = 30
    print("Old width,height:",frame_width,frame_height)
    frame_width=640
    frame_height=480
    o1=now.strftime("%Y-%m-%d_%H_%M_%S_%Z")
    p=os.path.join('.', 'dataOutput', o1,'outpy' + date_time + '.mp4')
    try:
      os.mkdir(os.path.dirname(p), mode=stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
    except FileExistsError:
      pass
    out = cv2.VideoWriter(p,cv2.VideoWriter_fourcc('a', 'v', 'c', '1'), fps, (frame_width,frame_height))

    try:
        while(shouldStop.get() == 0):
          ret, frame = cap.read()

          if ret == True: 

            # Write the frame into the file 'output.avi'
            out.write(frame)

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
          if duration.total_seconds() >= 2:
              lastFlush = now
              # Flush video
              out.release()
              print("Flushed the video")
              date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
              p=os.path.join('.', 'dataOutput',o1,'outpy' + date_time + '.mp4')
              out.open(p,cv2.VideoWriter_fourcc('a', 'v', 'c', '1'), fps, (frame_width,frame_height))
    except KeyboardInterrupt:
        print("Handing keyboardinterrupt")
        pass
    finally:    
        # When everything done, release the video capture and video write objects
        cap.release()
        out.release()

        # Closes all the frames
        #cv2.destroyAllWindows()

if __name__ == "__main__":
  run(AtomicInt(0))
