# https://opencv24-python-tutorials.readthedocs.io/en/latest/py_tutorials/py_feature2d/py_matcher/py_matcher.html

import numpy as np
import cv2
from matplotlib import pyplot as plt

#img1 = cv2.imread('box.png',0)          # queryImage
#img2 = cv2.imread('box_in_scene.png',0) # trainImage
img1 = cv2.imread('Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0013.png')
img2 = cv2.imread('Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0016.png')

# Initiate SIFT detector
sift = cv2.xfeatures2d.SIFT_create() # https://stackoverflow.com/questions/63949074/cv2-sift-causes-segmentation-fault  #cv2.SIFT()

# find the keypoints and descriptors with SIFT
kp1, des1 = sift.detectAndCompute(img1,None)
kp2, des2 = sift.detectAndCompute(img2,None)

# BFMatcher with default params
bf = cv2.BFMatcher()
matches = bf.knnMatch(des1,des2, k=2)

# Apply ratio test
good = []
for m,n in matches:
    if m.distance < 0.75*n.distance:
        good.append([m])

# cv2.drawMatchesKnn expects list of lists as matches.
img3 = None
img3 = cv2.drawMatchesKnn(img1,kp1,img2,kp2,good,img3,flags=2)

plt.imshow(img3),plt.show()
