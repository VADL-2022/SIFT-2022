import cv2
import numpy as np
from sys import argv
from math import sin, cos, pi, atan2, asin, sqrt, ceil
import copy
import src.python.General

idMat=[[1.0, 0.0, 0.0],
       [0.0, 1.0, 0.0],
       [0.0, 0.0, 1.0]]

imgOrig = cv2.imread(argv[1])
distort=argv[2] == '1' if len(argv) > 2 else False
if distort:
    #imgOrig = src.python.General.distortImage(imgOrig)
    pass
undistort=argv[3] == '1' if len(argv) > 3 else False
if undistort:
    imgOrig = src.python.General.undisortImage(imgOrig)
widthAndHeight=np.array(imgOrig.shape[:2])
# Swap #
temp=widthAndHeight[0]
widthAndHeight[0] = widthAndHeight[1]
widthAndHeight[1]=temp
# #
Mcurrent=copy.deepcopy(idMat)
Mcurrent=[[-0.18071, -0.77495, 573.08481],
          [0.77063, 0.06183, -34.54974],
          [-0.00005, -0.00003, 1.00000]]
inc=0.5/2
incTranslate=5
from enum import Enum
class ShrinkMode(Enum):
    Width = 1
    Height = 2
shrinkMode=ShrinkMode.Width
angle=0
angleInc=2
def updateRotation():
    Mcurrent[0][0] = cos(np.deg2rad(angle))
    Mcurrent[0][1] = -sin(np.deg2rad(angle))
    Mcurrent[1][0] = sin(np.deg2rad(angle))
    Mcurrent[1][1] = cos(np.deg2rad(angle))
while True:
    print(Mcurrent)
    tr = cv2.warpPerspective(imgOrig, np.matrix(Mcurrent), widthAndHeight)
    cv2.imshow('acc', tr)
    key = cv2.waitKey(0)
    # if key & 0xFF == ord('q'):
    #     exit(0)
    if False:
        pass
    elif key & 0xFF == ord('f'):
        # increase inc
        inc+=inc*0.1
        incTranslate+=incTranslate*0.1
    elif key & 0xFF == ord('g'):
        # decrease inc
        inc-=inc*0.1
    elif key & 0xFF == ord('m'):
        shrinkMode=ShrinkMode.Width if shrinkMode == ShrinkMode.Height else ShrinkMode.Height
        print(shrinkMode)
    # WASD Star Conflict controls
    # Transformations based on https://en.wikipedia.org/wiki/File:2D_affine_transformation_matrix.svg
    elif key & 0xFF == ord('w'): # Shrink image width or height
        if shrinkMode == ShrinkMode.Height:
            Mcurrent[1][1] -= inc
        elif shrinkMode == ShrinkMode.Width:
            Mcurrent[0][0] -= inc
    elif key & 0xFF == ord('s'): # Grow image
        if shrinkMode == ShrinkMode.Height:
            Mcurrent[1][1] += inc
        elif shrinkMode == ShrinkMode.Width:
            Mcurrent[0][0] += inc
    elif key & 0xFF == ord('a'): # Translate image left
        Mcurrent[0][2] -= incTranslate
    elif key & 0xFF == ord('d'): # Translate image right
        Mcurrent[0][2] += incTranslate
    elif key & 0xFF == ord('z'): # Translate image down (should be Shift but not sure how to do that)
        Mcurrent[1][2] += incTranslate
    elif key & 0xFF == ord(' '): # Translate image up
        Mcurrent[1][2] -= incTranslate
    elif key & 0xFF == ord('q'): # Rotate image left
        angle -= angleInc
        updateRotation()
    elif key & 0xFF == ord('e'): # Rotate image right
        angle += angleInc
        updateRotation()
