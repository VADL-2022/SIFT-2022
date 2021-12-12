# https://learnopencv.com/read-write-and-display-a-video-using-opencv-cpp-python/

import cv2
import numpy as np
from datetime import datetime
now = datetime.now() # current date and time

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
out = cv2.VideoWriter('./dataOutput/outpy' + date_time + '.mp4',cv2.VideoWriter_fourcc('a', 'v', 'c', '1'), fps, (frame_width,frame_height))

try:
    while(True):
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
except KeyboardInterrupt:
    print("Handing keyboardinterrupt")
    pass
finally:    
    # When everything done, release the video capture and video write objects
    cap.release()
    out.release()

    # Closes all the frames
    #cv2.destroyAllWindows()
