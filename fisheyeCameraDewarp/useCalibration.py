import cv2 as cv
import cv2
import numpy as np
import sys
import glob

# The calibration data #
mtx = np.matrix([[7.35329235e+02, 0.00000000e+00, 1.28117269e+03],
                [0.00000000e+00, 7.40147960e+02, 9.90415290e+02],
                [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist = np.matrix([[-0.25496947,  0.07321429, -0.00104747,  0.00103111, -0.00959384]])
# #

# https://docs.opencv.org/3.4/dc/dbb/tutorial_py_calibration.html
img = cv.imread('images/Fri 11 Mar 15_01_52 EST 2022.jpg')
h, w = img.shape[:2]
newcameramtx, roi = cv.getOptimalNewCameraMatrix(mtx, dist, (w,h), 1, (w,h))

useFullImageInsteadOfROI = sys.argv[2] == '1' if len(sys.argv) > 2 else True
k=mtx
if useFullImageInsteadOfROI:
    # https://stackoverflow.com/questions/34316306/opencv-fisheye-calibration-cuts-too-much-of-the-resulting-image
    nk = k.copy()
    nk[0,0]=k[0,0]/2
    nk[1,1]=k[1,1]/2
    print(w,h)
    # Notice the cv.fisheye.initUndistortRectifyMap being used instead of cv.initUndistortRectifyMap!:
    mapx, mapy = cv.fisheye.initUndistortRectifyMap(mtx, dist[:-1], None, nk, (w,h), cv2.CV_16SC2)
else:
    # Prepare reusable undistort mapping thing
    mapx, mapy = cv.initUndistortRectifyMap(mtx, dist, None, newcameramtx, (w,h), 5)

useCam = sys.argv[1] == '1' if len(sys.argv) > 1 else False
if useCam:
    cap = cv.VideoCapture(0)
else:
    images = glob.glob('./images/*.jpg')
    iter_ = iter(images)
#for fname in images:
while cap.isOpened() if useCam else True:
    if useCam:
        ret, img = cap.read()
        if ret == False:
            print("ret == False")
            continue
    else:
        try:
            fname = next(iter_)
        except StopIteration:
            break
        img = cv.imread(fname)
    
    # undistort
    if useFullImageInsteadOfROI:
        dst = cv.remap(img, mapx, mapy, cv.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
    else:
        dst = cv.remap(img, mapx, mapy, cv.INTER_LINEAR)
    # crop the image
    if not useFullImageInsteadOfROI:
        x, y, w, h = roi
        print("roi:", roi)
        dst = dst[y:y+h, x:x+w]
    cv.imshow('calibresult.png', dst)
    #cv.imwrite('calibresult.png', dst)
    cv.waitKey(0)

cv.destroyAllWindows()
