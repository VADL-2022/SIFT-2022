import cv2
import numpy as np

# https://stackoverflow.com/questions/34316306/opencv-fisheye-calibration-cuts-too-much-of-the-resulting-image

k = np.matrix([[ 329.75951163,    0.0,          422.36510555],
               [   0.0,          329.84897388,  266.45855056],
               [   0.0,            0.0,            1.0        ]])

d = np.matrix([[ 0.04004325],
               [ 0.00112638],
               [ 0.01004722],
               [-0.00593285]])

img = cv2.imread('download.jpeg')
map1, map2 = cv2.fisheye.initUndistortRectifyMap(k, d, np.eye(3), k, (800,600), cv2.CV_16SC2)
nemImg = cv2.remap( img, map1, map2, interpolation=cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
cv2.imshow('1',nemImg)
cv2.waitKey(0)

nk = k.copy()
nk[0,0]=k[0,0]/2
nk[1,1]=k[1,1]/2
# Just by scaling the matrix coefficients!

map1, map2 = cv2.fisheye.initUndistortRectifyMap(k, d, np.eye(3), nk, (800,600), cv2.CV_16SC2)  # Pass k in 1st parameter, nk in 4th parameter
nemImg = cv2.remap( img, map1, map2, interpolation=cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
cv2.imshow('2',nemImg)
cv2.waitKey(0)
