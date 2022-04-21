# Based on https://opencv24-python-tutorials.readthedocs.io/en/latest/py_tutorials/py_feature2d/py_matcher/py_matcher.html

from __future__ import annotations
import numpy as np
import cv2
from matplotlib import pyplot as plt
from src.python.General import shouldDiscardImage, undisortImage
import src.python.General
import os
from pathlib import Path
import sys
from datetime import datetime
from src.python import GridCell
import re
import timeit
from enum import Enum

class Action(Enum):
    Break = 1
    Continue = 2


class EarlyExitException(Exception):
    pass

#img1 = cv2.imread('box.png',0)          # queryImage
#img2 = cv2.imread('box_in_scene.png',0) # trainImage

# Initiate SIFT detector
# https://docs.opencv.org/3.4/d7/d60/classcv_1_1SIFT.html for params:
# Defaults #
# nfeatures = 0
# nOctaveLayers = 3
# contrastThreshold = 0.04
# edgeThreshold = 10
# sigma = 1.6
# #
# Custom #
nfeatures = 0
nOctaveLayers = 10
contrastThreshold = 0.02
edgeThreshold = 10
sigma = 0.8
# #
sift = cv2.xfeatures2d.SIFT_create(nfeatures, nOctaveLayers, contrastThreshold, edgeThreshold, sigma) # https://stackoverflow.com/questions/63949074/cv2-sift-causes-segmentation-fault  #cv2.SIFT()
#sift = cv2.ORB_create()

# BFMatcher with default params
bf = cv2.BFMatcher()

isMain=__name__ == "__main__"
if __name__ == "__main__":
    # Config #
    mode = 1
    grabMode=int(sys.argv[1]) if len(sys.argv) > 1 else 2 # 1 for video, 0 for photos, 2 for provide list of images manually
    shouldRunSkyDetection=sys.argv[2] == '1' if len(sys.argv) > 2 else True
    shouldRunUndistort=sys.argv[3] == '1' if len(sys.argv) > 3 else False
    skip=int(sys.argv[4]) if len(sys.argv) > 4 else 0
    videoFilename=sys.argv[5] if len(sys.argv) > 5 else None
    showPreviewWindow=sys.argv[6] == '1' if len(sys.argv) > 6 else True
    frameSkip=int(sys.argv[7]) if len(sys.argv) > 7 else 1
    imgs=None
    if grabMode==1:
        reader=cv2.VideoCapture(
            # Drone tests from 3-28-2022: #
             'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_18_34_07_CDT/output.mp4' # GOOD
            # 'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_18_39_25_CDT/output.mp4' # CHALLENGING; a bit offset
            #'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_18_46_26_CDT/output.mp4' #<--!!!!!!!!!!!!!!! WOAH! shockingly accurate result
            #'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_18_50_08_CDT/output.mp4' # also good, but recovery is a little late
            #'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_18_54_34_CDT/output.mp4' # TOUGHEST ONE

            # hard one:
            #'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_19_00_01_CDT/output.mp4' # WOW... actually not bad even though it hits the tower thing. maybe its higher fps is the key for opencv sift?
            # #

            #'Data/DroneTests/DroneTest_3-28-2022/2022-03-28_18_39_25_CDT/output.mp4'
            #'quadcopterFlight/live2--.mp4' # our old standby
            #'/Volumes/MyTestVolume/Projects/DataRocket/files_sift1_videosTrimmedOnly_fullscale1/Derived/live18_downwards_test/output.mp4'
        ) if videoFilename is None else cv2.VideoCapture(videoFilename)
    elif grabMode==0:
        import subprocess
        #p=subprocess.run(["../compareNatSort/compareNatSort", "../Data/fullscale1/Derived/SIFT/ExtractedFrames", ".png"], capture_output=True)
        p=subprocess.run(["compareNatSort/compareNatSort",
                          "Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated"
                          # '/Users/sebastianbond/Desktop/sdCardImages/sdCardN64RaspberryPiImage/2022-03-21_19_02_14_CDT'
                          #'/Users/sebastianbond/Desktop/sdCardImages/sdCardN64RaspberryPiImage/2022-03-21_18_47_47_CDT'
                          , ".png"], capture_output=True)
    elif grabMode==2:
        imgs=[
            #'cnn-registration/img/Screen Shot 2022-03-29 at 2.13.17 PM.png',
            #'Data/fullscale1/Derived/SIFT/ExtractedFrames_undistorted/thumb0127.png',
            #'Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0127.png',

            'Data/fullscale1/Derived/SIFT/SatelliteMatchingTesting/thumb0005.png',
            'Data/fullscale1/Derived/SIFT/SatelliteMatchingTesting/Screen Shot 2022-03-29 at 2.13.17 PM_.png',
            'Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0127.png',
        ]
    elif grabMode==3:
        import subprocess
        p=subprocess.run(["compareNatSort/compareNatSort",
                          "Data/fullscale1/Derived/SIFT/ExtractedFrames"
                          , ".png"], capture_output=True)
    # End Config #

# This function is from https://www.programcreek.com/python/example/110698/cv2.estimateAffinePartial2D
def find_homography(keypoints_pic1, keypoints_pic2, matches) -> (List, np.float32, np.float32):
        # Find an Homography matrix between two pictures
        # From two list of keypoints and a list of matches, extrat
        # A list of good matches found by RANSAC and two transformation matrix (an homography and a rigid homography/affine)

        # Instanciate outputs
        good = []

        # Transforming keypoints to list of points
        #print(keypoints_pic1[matches[0].queryIdx])
        #print(matches[0])
        #print(cv2.KeyPoint_convert(keypoints_pic1[matches[0].queryIdx]))
        src_pts = np.float32([keypoints_pic1[m.queryIdx].pt for m in matches]).reshape(-1, 1, 2)
        dst_pts = np.float32([keypoints_pic2[m.trainIdx].pt for m in matches]).reshape(-1, 1, 2)
        #print("src_pts:", src_pts)

        # Find the transformation between points
        #transformation_matrix, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)
        transformation_matrix, mask = cv2.estimateAffine2D(src_pts, dst_pts, np.array([]), cv2.RANSAC, 5.0)
        if transformation_matrix is not None:
            # https://stackoverflow.com/questions/3881453/numpy-add-row-to-array
            transformation_matrix = np.append(transformation_matrix, [[0.0,0.0,1.0]], axis=0) # Make affine into perspective matrix
            print(transformation_matrix)

        # Compute a rigid transformation (without depth, only scale + rotation + translation)
        transformation_rigid_matrix, rigid_mask = cv2.estimateAffinePartial2D(src_pts, dst_pts)

        # Get a mask list for matches = A list that says "This match is an in/out-lier"
        matchesMask = mask.ravel().tolist()

        # Filter the matches list thanks to the mask
        for i, element in enumerate(matchesMask):
            if element == 1:
                good.append(matches[i])

        return good, transformation_matrix, transformation_rigid_matrix

def grabImage(imgName, i, firstImage, skip=False):
    def grabInternal(imgName):
        if grabMode == 1:
            ret,frame = reader.read()
            if frame is None:
                print("knn matcher: Done processing images")
                e = EarlyExitException()
                raise e
            return frame
        else:
            if isinstance(imgName, bytes):
                imgName=imgName.decode('utf-8')
            elif not isinstance(imgName, str):
                # Assume cv matrix
                print("grabInternal: cv matrix for image", i)
                return imgName
            #imgNameNew = os.path.normpath(imgName) # DOESN"T WORK BUT WORKS IN THE PYTHON PROMPT?! IMPOSSIBLE!!
            #imgNameNew = re.sub(r'/+', lambda matchobj: '/', imgName) # same as the above, not working here but works in the prompt
            #imgNameNew = os.path.join(os.path.dirname(imgName), os.path.basename(imgName)) # same as above
            # Found out the reason for the above: the actual string is different from what is printed!:
            # Python 3.7.11 (default, Jun 28 2021, 17:43:59)
            # [GCC 9.3.0] on linux
            # Type "help", "copyright", "credits" or "license" for more information.
            # (InteractiveConsole)
            # >>> imgName
            # 'dataOutput/2022-04-16_02_19_40_CDT/\x00/firstImage0.png'
            # >>> imgNameNew
            # 'dataOutput/2022-04-16_02_19_40_CDT/\x00firstImage0.png'
            # >>>
            #imgNameNew = os.path.dirname(imgName).rstrip('\x00/') + '/' os.path.basename(imgName).lstrip('\x00/')
            imgNameNew = imgName
            #import code
            #code.InteractiveConsole(locals=locals()).interact()
            #print(imgName, imgNameNew)
            print(imgNameNew)
            # load the image and convert it to grayscale
            image=cv2.imread(imgNameNew)
            #image=cv2.imread('dataOutput/2022-04-16_02_02_13_CDT/firstImage0.png') # THIS WORKS BUT NOT THE ABOVE when imgNameNew is as mentioned above
            #print("image:",image,shouldRunSkyDetection)
            return image

    for j in range(0,frameSkip):
        frame = grabInternal(imgName)
        if skip:
            return None, True, None
    if frame is None:
        return None, False, None
    if True: #if firstImage is not None:
        # Convert to same format
        # scaling_factor=1.0/255.0 # technically this works only for uint8-based images
        # if frame.dtype != np.uint8:
        #     print("Error but continuing: incorrect dtype for frame:",frame.dtype)
        # if frame.dtype != np.float32:
        #     frame = np.float32(frame)*scaling_factor
        
        scaling_factor=255.0 # technically this works only for float32-based images
        if frame.dtype != np.float32 and frame.dtype != np.uint8:
            print("Error but continuing: incorrect dtype for frame:",frame.dtype)
        if frame.dtype != np.uint8:
            frame = np.uint8(frame)*scaling_factor
    if shouldRunUndistort:
        #frame = undisortImage(frame)
        frameOrig = frame.copy()
        frame = src.python.General.undistortImg(frame)
    else:
        frameOrig = frame
    greyscale = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    if shouldRunSkyDetection and shouldDiscardImage(greyscale, i):
        print("Discarded image", i)
        return None, True, greyscale
    else:
        return (frame, frameOrig), False, greyscale


# https://medium.com/swlh/youre-using-lerp-wrong-73579052a3c3
def lerp(a, b, t):
    return a + (b-a) * t

idMat=np.matrix([[1.0, 0.0, 0.0],
                 [0.0, 1.0, 0.0],
                 [0.0, 0.0, 1.0]])
def showLerpController(firstImage, M, key_='a', idMat=idMat):
    img2=firstImage
    Mcurrent = idMat.copy()
    #inc=0.005*2
    inc=0.005/2
    incOrig=inc
    while True:
        imgOrig=img2
        print(imgOrig.shape[:2])
        widthAndHeight=np.array(imgOrig.shape[:2])
        temp=widthAndHeight[0]
        widthAndHeight[0] = widthAndHeight[1]
        widthAndHeight[1]=temp
        print(widthAndHeight)
        img = cv2.warpPerspective(imgOrig, np.linalg.pinv(Mcurrent), widthAndHeight)
        cv2.imshow('lerp controller',img)
        cv2.resizeWindow('', widthAndHeight[0], widthAndHeight[1])
        for i in range(0, len(Mcurrent)):
            x = Mcurrent[i]
            for j in range(0, len(x)):
                y = x[j]
                Mcurrent[i][j] = lerp(y, M[i][j], inc)
                #inc*=1.5
        key = cv2.waitKey(0)
        if key & 0xFF == ord('q'):
            exit(0)
        elif key & 0xFF == ord(key_):
            continue
        elif key & 0xFF == ord('f'): # Faster
            inc+=inc*0.1
        elif key & 0xFF == ord('g'): # Faster
            inc+=0.001
        elif key & 0xFF == ord('s'): # Slower
            inc-=0.001*2
        elif key & 0xFF == ord('x'): # Slower
            inc-=inc*0.1
        elif key & 0xFF == ord('i'): # Identity
            Mcurrent=idMat.copy()
            inc = incOrig
            img=imgOrig.copy()
        elif key & 0xFF == ord('m'): # Use matrix
            Mcurrent=M.copy()
        else:
            break
def showLandingPos(firstImage, M, key_='l', idMat=idMat):
    img = firstImage.copy()
    # 4 corner points
    height,width=img.shape[:2]
    pts = np.array([[(0,0), (width,0), (width,height), (0,height)]], dtype=np.float32)
    dstPts = np.zeros_like(pts)
    print(pts, dstPts)
    dstPts = cv2.perspectiveTransform(pts, M, dstPts)
    print(dstPts)
    dstPts=dstPts[0] # It was wrapped by an extra array
    # Show average
    dstMidpoint = np.mean(dstPts, axis=0) # https://stackoverflow.com/questions/15819980/calculate-mean-across-dimension-in-a-2d-array
    # undistort midpoint
    #undistortPoints = cv2.fisheye.undistortPoints # gives error: `cv2.error: OpenCV(4.5.2) /private/var/folders/yg/9244vmfs33v7l8m1wm7glkp40000gn/T/nix-build-opencv-4.5.2.drv-0/source/modules/calib3d/src/fisheye.cpp:331: error: (-215:Assertion failed) D.total() == 4 && K.size() == Size(3, 3) && (K.depth() == CV_32F || K.depth() == CV_64F) in function 'undistortPoints'`
    #undistortPoints = cv2.undistortPoints
    dstMidpointUndist = src.python.General.undistortPoints(np.array([[dstMidpoint*(src.python.General.w/640)]], dtype=np.float32))[0][0]*(640/src.python.General.w)
    #dstMidpointUndist = undistortPoints(np.array([[dstMidpoint*(src.python.General.w/640)]], dtype=np.float32), src.python.General.k, src.python.General.dist)[0][0]*(640/src.python.General.w)
    print("dstMidpointUndist:",dstMidpointUndist)
    cv2.circle(img, list(map(int, dstMidpoint)), 3, (255, 0, 0), 3)
    cv2.circle(img, list(map(int, dstMidpointUndist)), 3, (255, 255, 0), 3)
    cv2.circle(img, list(map(int, np.array([width/2,height/2])+dstMidpointUndist-dstMidpoint)), 3, (0, 255, 255), 3)
    if len(dstPts) > 1:
        for i in range(0, len(dstPts)-1):
            print(dstPts[i])
            cv2.line(img, list(map(int, dstPts[i])), list(map(int, dstPts[i+1])), (255,0,0), 2)
        cv2.line(img, list(map(int, dstPts[0])), list(map(int, dstPts[len(dstPts)-1])), (255,0,0), 2)
    # Get landing grid cell #
    cv2.putText(img,str(GridCell.getGridCellIdentifier(width, height, dstMidpoint[0], dstMidpoint[1])),(5,5+25),cv2.FONT_HERSHEY_SIMPLEX,1,(0,0,255),2)  #text,coordinate,font,size of text,color,thickness of font   # https://stackoverflow.com/questions/16615662/how-to-write-text-on-a-image-in-windows-using-python-opencv2
    # #
    cv2.imshow('landingPos', img)
    key = cv2.waitKey(0)
    return img, key
    
def run(pSave=None):
    if not showPreviewWindow:
        if pSave is None:
            pSave="dataOutput"

            now = datetime.now() # current date and time
            date_time = now.strftime("knnMatcher_%m_%d_%Y_%H_%M_%S")
            pSave = os.path.join(pSave, date_time)

            Path(pSave).mkdir(parents=True, exist_ok=True) # https://stackoverflow.com/questions/273192/how-can-i-safely-create-a-nested-directory
    else:
        pSave=None
    
    global imgs
    if grabMode == 1:
        totalFrames = int(reader.get(cv2.CAP_PROP_FRAME_COUNT))
        imgs = [None]*totalFrames
    elif grabMode == 0 or grabMode == 3:
        imgs=p.stdout.split(b'\n')
    i = 0
    img1Pair = None
    discarded = True # Assume True to start with
    greyscale = None
    tr = None
    
    imgs_iter = iter(imgs)
    next(imgs_iter) # Skip first image
    
    if len(imgs) > 0:
        # Skip images
        for j in range(0, skip):
            i += 1
            #print('skip')
            img1Pair, discarded, greyscale = grabImage(imgs[i], i, None, True)
        #exit(0)

        while img1Pair is None or discarded:
            firstImageFilename=imgs[i]
            img1Pair, discarded, greyscale = grabImage(imgs[i], i, None)
            i+=1
        firstImage = img1Pair[0]
        if not showPreviewWindow or __name__ == "__main__":
            # Save first image
            cv2.imwrite(os.path.join(pSave, "firstImage0.png"), firstImage)
        firstImageOrig=img1Pair[1] # undistorted etc.
        if showPreviewWindow:
            cv2.imshow('firstImage (index ' + str(i-1) + ')', firstImage)
            cv2.waitKey(0)
        
        mask2 = np.zeros_like(greyscale)
        img1=img1Pair[0]
        hOrig, wOrig = img1.shape[:2]
        hOrig_=hOrig
        wOrig_=wOrig
        xc = int(wOrig / 2)
        yc = int(hOrig / 2)
        radius2 = int(hOrig * 0.45)
        test_mask=cv2.circle(mask2, (xc,yc), radius2, (255,255,255), -1) # https://stackoverflow.com/questions/42346761/opencv-python-feature-detection-how-to-provide-a-mask-sift
        if showPreviewWindow:
            cv2.imshow('test_mask', test_mask)
            cv2.waitKey(0)
    
        kp1, des1 = sift.detectAndCompute(img1,mask = test_mask) # Returns keypoints and descriptors
    else:
        print("No images")

    def flushMatAndScaledImage(pSave, i, tr, acc):
        if pSave is None:
            return
        if not showPreviewWindow or __name__ == "__main__":
            if tr is not None:
                # Save transformation
                cv2.imwrite(os.path.join(pSave, "scaled" + str(i) + ".png"), tr)
                # Save matrix
                path1=os.path.join(pSave, "scaled" + str(i) + ".matrix0.txt")
                #cv2.imwrite(path1, acc)
                with open(path1, 'w') as f:
                    f.write("[" + str(acc[0][0]) + ", " + str(acc[0][1]) + ", " + str(acc[0][2]) + ";\n")
                    f.write("" + str(acc[1][0]) + ", " + str(acc[1][1]) + ", " + str(acc[2][2]) + ";\n")
                    f.write("[" + str(acc[2][0]) + ", " + str(acc[2][1]) + ", " + str(acc[2][2]) + "]\n")
            else:
                print("tr is None")

    #acc = np.matrix([[1,0,0],[0,1,0],[0,0,1]])
    acc = np.matrix([[1.0,0.0,0.0],[0.0,1.0,0.0],[0.0,0.0,1.0]])
    #i = 1 # (Skipped first image)
    # For each image, run SIFT and match with previous image:
    def runInternal(imgNameOrImg):
        imgName=imgNameOrImg
        nonlocal acc
        nonlocal hOrig
        nonlocal wOrig
        nonlocal i
        nonlocal img1
        nonlocal des1
        nonlocal kp1
        nonlocal greyscale
        def waitForInput(img2, firstImage, skipWaitKeyUsingKey=None):
            if not showPreviewWindow:
                return
            waitAmount = 1 if i < len(imgs) - 20 or not isinstance(reader, cv2.VideoCapture) else 0
            if waitAmount == 0:
                print("Press a key to continue")
                if i == len(imgs) - 1:
                    print("(Last image)")
            if skipWaitKeyUsingKey is None:
                k=cv2.waitKey(waitAmount)
            else:
                k=skipWaitKeyUsingKey
            applyMatKey='a'
            showLandingPosKey='l'
            if k & 0xFF == ord('q'):
                if isMain:
                    exit(0)
                else:
                    e = EarlyExitException()
                    e.acc = acc
                    e.w=wOrig_
                    e.h=hOrig_
                    e.firstImage=firstImage
                    e.firstImageFilename=firstImageFilename
                    raise e
            elif k & 0xFF == ord(applyMatKey) and img2 is not None: # Apply matrix with lerp
                showLerpController(firstImage, acc, applyMatKey)
            elif k & 0xFF == ord(showLandingPosKey) and img2 is not None: # Show landing position
                img, key = showLandingPos(firstImage, acc, showLandingPosKey)
                if k & 0xFF == ord(applyMatKey):
                    waitForInput(img2, firstImage, applyMatKey)

        try:
            img2Pair, discarded, greyscale = grabImage(imgName, i, firstImage)
            print(i,"$$$$$$$$$$$$$$$$$$$$$$$$$")
            #input()
        except EarlyExitException as e:
                e.acc = acc
                e.w=wOrig_
                e.h=hOrig_
                e.firstImage=firstImage
                e.firstImageFilename=firstImageFilename
                raise e
        if img2Pair is None and not discarded:
            print("img2 was None")
            if showPreviewWindow:
                cv2.waitKey(0)
            return Action.Break
        if img2Pair is None and discarded:
            waitForInput(None, firstImage)
            i += 1
            return Action.Continue #  # Keep img1 as the previous image so we can match it next time
        img2=img2Pair[0]
        hOrig, wOrig = img2.shape[:2]

        # find the keypoints and descriptors with SIFT
        kp2=None
        des2=None
        def fn():
            nonlocal kp2
            nonlocal des2
            kp2, des2 = sift.detectAndCompute(img2,mask = test_mask) # Returns keypoints and descriptors
        print("sift.detectAndCompute() took", timeit.timeit(lambda: fn(), number=1), "seconds")

        # Find matches
        matches = None
        good = None
        good_flatList = None
        good_old = None
        def fn2():
            nonlocal matches
            nonlocal good
            nonlocal good_flatList
            nonlocal good_old
            
            matches = bf.knnMatch(des2,des1, k=2) # Swap order of des1,des2 because want to find first image in the second

            # Apply ratio test
            good = []
            good_flatList = []
            for m,n in matches:
                if m.distance < 0.75*n.distance:
                    good.append([m])
                    good_flatList.append(m)

                    # cv2.drawMatchesKnn expects list of lists as matches.
                    #img3 = None
                    #img3 = cv2.drawMatchesKnn(img1,kp1,img2,kp2,good,img3,flags=2)
            good_old = good
        print("matching took", timeit.timeit(lambda: fn2(), number=1), "seconds")

        transformation_matrix = None
        transformation_rigid_matrix = None
        continue_=False
        def fn3():
            nonlocal transformation_matrix
            nonlocal transformation_rigid_matrix
            nonlocal i
            nonlocal continue_
            nonlocal good
            try:
                good, transformation_matrix, transformation_rigid_matrix = find_homography(kp2, kp1, good_flatList) # Swap order of kp1, kp2 because want to find first image in the second
            except cv2.error:
                print("Error in find_homography, probably not enough keypoints:", sys.exc_info()[1])
                if showPreviewWindow:
                    cv2.imshow('bad',img2);
                waitForInput(None, firstImage)
                i+=1
                continue_=True  # Keep img1 as the previous image so we can match it next time
        print("find_homography took", timeit.timeit(lambda: fn3(), number=1), "seconds")
        if continue_:
            return Action.Continue
        
        if transformation_matrix is None:
            print("transformation_matrix was None")
            waitForInput(None, firstImage)
            i+=1
            return Action.Continue # Keep img1 as the previous image so we can match it next time
            # cv2.waitKey(0)
            # break
        acc *= transformation_matrix
        accOld = acc.copy()
        # #print(acc[0])
        # acc[0][0] *= 0.9
        # #print(acc[0])
        # acc[0][1] *= 0.9
        # acc[1][0] *= 0.9
        # acc[0][1] *= 0.9
        print("acc:", accOld, acc)

        if showPreviewWindow:
            img3 = None
            img3 = cv2.drawMatchesKnn(img2,kp2,img1,kp1,good_old if mode==0 else list(map(lambda x: [x], good)),outImg=img3,flags=2)
            cv2.imshow('matching',img3);
            tr = cv2.warpPerspective(img1, transformation_matrix, (wOrig, hOrig))
            cv2.imshow('current transformation',tr)
        tr = cv2.warpPerspective(firstImage, np.linalg.pinv(acc), (wOrig, hOrig))
        if showPreviewWindow:
            cv2.imshow('acc',tr);
            waitForInput(img2, firstImage)
            if i % 4 == 0:
                print("Flushing matrix and scaled image")
                flushMatAndScaledImage(pSave, i, tr, acc)
        else:
            if i % 4 == 0:
                print("Flushing matrix")
                flushMatAndScaledImage(pSave, i, None, acc)
        #plt.imshow(img3),plt.show()
        
        # Save current keypoints as {the prev keypoints for next iteration}
        kp1=kp2
        des1=des2
        img1=img2
        i+=1
        return None
    try:
        if len(imgs) > 1:
            for imgName in imgs_iter:
                ret = runInternal(imgName)
                if ret == Action.Continue:
                    continue
                elif ret == Action.Break:
                    break
        else:
            # Must be real video capture object
            while True:
                ret = runInternal(reader.read())
                if ret == Action.Continue:
                    continue
                elif ret == Action.Break:
                    break
    finally:
        flushMatAndScaledImage(pSave, i, None, acc)
    return acc, wOrig_, hOrig_, firstImage, firstImageOrig, firstImageFilename

if __name__ == "__main__":
    run()
