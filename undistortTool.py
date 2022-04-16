import cv2
import src.python.General
#from src.python.General import shouldDiscardImage, undisortImage
import os
from pathlib import Path
import sys

imgName=sys.argv[1] #imgName = 'Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0089.png'
src.python.General.useFullImageInsteadOfROI=sys.argv[2] == '1' if len(sys.argv) > 2 else False
img=src.python.General.undisortImage(cv2.imread(imgName))
cv2.imshow('',img)
k=cv2.waitKey(0)
if k & 0xFF == ord('w'): # Write
    # Write/save the undistort:
    # Make the directory
    p="undistortedImages"
    Path(p).mkdir(exist_ok=True) # https://stackoverflow.com/questions/273192/how-can-i-safely-create-a-nested-directory

    # Save
    name,ext=os.path.splitext(os.path.basename(imgName))
    cv2.imwrite(os.path.join(p, name + ".undistorted" + ext), img)
