import cv2
import numpy as np
import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'driver')
sys.path.append(p)
import driver.satelliteImageMatching

# Grab sift on the way down transform #
import knn_matcher2

showPreviewWindow = sys.argv[4] == '1' if len(sys.argv) > 4 else True
skip=int(sys.argv[5]) if len(sys.argv) > 5 else 0
frameSkip=int(sys.argv[6]) if len(sys.argv) > 6 else 1
sky_detection = sys.argv[7] == '1' if len(sys.argv) > 7 else False
def runOnTheWayDown(videoFilename):
    knn_matcher2.mode = 1
    knn_matcher2.grabMode = 1
    knn_matcher2.shouldRunSkyDetection = sky_detection
    knn_matcher2.shouldRunUndistort = True #False
    knn_matcher2.skip = skip
    knn_matcher2.videoFilename = None
    knn_matcher2.showPreviewWindow = showPreviewWindow
    knn_matcher2.reader = cv2.VideoCapture(videoFilename)
    knn_matcher2.frameSkip = frameSkip #40#1#5#10#20
    knn_matcher2.waitAmountStandard = None
    rets = knn_matcher2.run()
    return rets
videoFilename=sys.argv[1] if len(sys.argv) > 1 else 'Data/fullscale3/sift2/2022-04-16_12_08_35_CDT/output.mp4'
firstImageFilename=sys.argv[2] if len(sys.argv) > 2 else "Data/fullscale3/sift2/2022-04-16_12_08_35_CDT/firstImage0.png"
def evalMatrix(matStr):
    import subprocess
    evalThisDotStdout=subprocess.run(['bash', '-c', "echo \"" + matStr + "\" | sed 's/^/[/' | sed -E 's/.$/]&/' | sed -E 's/;/,/'"], capture_output=True, text=True)
    stdout_=evalThisDotStdout.stdout
    print("about to eval (stderr was", evalThisDotStdout.stderr, "):", stdout_)
    return np.matrix(eval(stdout_))
#useExistingMatrix=evalMatrix(sys.argv[3]) if len(sys.argv) > 3 else None
useExistingMatrix=None
# if len(sys.argv) > 3:
#     useExistingMatrix=np.matrix([[-7.14492583e-03,  1.84838683e-03,  6.24101798e+02],
#                                  [ 5.11659590e-02, -7.78842513e-02,  3.58747568e+02],
#                                  [ 0.00000000e+00,  0.00000000e+00,  1.00000000e+00]])
# else:
#     useExistingMatrix = None
firstImage=cv2.imread(firstImageFilename)
#print(firstImage)
if useExistingMatrix is None:
    try:
        # NOTE: first image given should match what the sky detector chooses so we re-set firstImage and firstImageFilename here
        accMat, w, h, firstImage, firstImageOrig, firstImageFilename = runOnTheWayDown(videoFilename)
    except knn_matcher2.EarlyExitException as e:
        accMat = e.acc
        w=e.w
        h=e.h
        firstImage=e.firstImage
        firstImageFilename=e.firstImageFilename
    print("Possibly new firstImageFilename:", firstImageFilename)
else:
    accMat = useExistingMatrix
    w=640
    h=480
# #

m0Res=cv2.warpPerspective(firstImage, np.linalg.pinv(accMat), (w,h))
cv2.imshow('sift on the way down',m0Res)
gridID, m0, m1, mc, firstImage_, firstImageOrig, firstImageFilename_=driver.satelliteImageMatching.run(accMat,
                                                                                                       firstImageOrig # not undistorted
                                                                                                       # firstImage # distorted
                                                                                       , 640/2, 480/2, showPreviewWindow)
#assert(m0==accMat)

m1Res=cv2.warpPerspective(firstImage, np.linalg.pinv(accMat)*(m1), (w,h)) # FIXME: w,h correct? ask emfinger
cv2.imshow('sift -> intermediate', m1Res)

mcRes=cv2.warpPerspective(firstImage, np.linalg.pinv(accMat)*(m1)*mc, (w,h)) # FIXME: w,h correct? ask emfinger
cv2.imshow('intermediate -> satellite', mcRes)
print("Grid identifier:", gridID)
while True:
    cv2.waitKey(0)
