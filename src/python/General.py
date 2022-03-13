import cv2
import numpy as np

# The calibration data #
mtx = np.matrix([[7.35329235e+02, 0.00000000e+00, 1.28117269e+03],
                [0.00000000e+00, 7.40147960e+02, 9.90415290e+02],
                [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist = np.matrix([[-0.25496947,  0.07321429, -0.00104747,  0.00103111, -0.00959384]])
# #

k=mtx
# https://stackoverflow.com/questions/34316306/opencv-fisheye-calibration-cuts-too-much-of-the-resulting-image
nk = k.copy()
nk[0,0]=k[0,0]/2
nk[1,1]=k[1,1]/2
mapx = None
mapy = None

w, h = (2592, 1944)
#h, w = image.shape[:2]
# Notice the cv.fisheye.initUndistortRectifyMap being used instead of cv.initUndistortRectifyMap!:
mapx, mapy = cv2.fisheye.initUndistortRectifyMap(mtx, dist[:-1], None, nk, (w,h), cv2.CV_16SC2)
        
def shouldDiscardImage(greyscaleImage):
    return False # temp

def undisortImage(image):
    hOrig, wOrig = image.shape[:2]
    image = cv2.resize(image, (w,h))
    image = cv2.remap(image, mapx, mapy, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
    image = cv2.resize(image, (wOrig,hOrig))
    return image
