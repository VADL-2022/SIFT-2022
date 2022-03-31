# Based on https://opencv24-python-tutorials.readthedocs.io/en/latest/py_tutorials/py_feature2d/py_matcher/py_matcher.html

from __future__ import annotations
import numpy as np
import cv2
from matplotlib import pyplot as plt
from src.python.General import shouldDiscardImage

#img1 = cv2.imread('box.png',0)          # queryImage
#img2 = cv2.imread('box_in_scene.png',0) # trainImage

# Initiate SIFT detector
sift = cv2.xfeatures2d.SIFT_create() # https://stackoverflow.com/questions/63949074/cv2-sift-causes-segmentation-fault  #cv2.SIFT()

# BFMatcher with default params
bf = cv2.BFMatcher()

# Config #
mode = 1
grabMode=1 # 1 for video, 0 for photos
if grabMode==1:
    reader=cv2.VideoCapture(
        # Good drone tests:
        '/Users/sebastianbond/Desktop/SeniorSemester2/RocketTeam/DroneTest_3-28-2022/2022-03-28_18_46_26_CDT/output.mp4'
        #'/Users/sebastianbond/Desktop/SeniorSemester2/RocketTeam/DroneTest_3-28-2022/2022-03-28_18_39_25_CDT/output.mp4'
    )
else:
    import subprocess
    #p=subprocess.run(["../compareNatSort/compareNatSort", "../Data/fullscale1/Derived/SIFT/ExtractedFrames", ".png"], capture_output=True)
    p=subprocess.run(["compareNatSort/compareNatSort",
                      "Data/fullscale1/Derived/SIFT/ExtractedFrames"
                      # '/Users/sebastianbond/Desktop/sdCardImages/sdCardN64RaspberryPiImage/2022-03-21_19_02_14_CDT'
                      #'/Users/sebastianbond/Desktop/sdCardImages/sdCardN64RaspberryPiImage/2022-03-21_18_47_47_CDT'
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

        # Find the transformation between points
        transformation_matrix, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)

        # Compute a rigid transformation (without depth, only scale + rotation + translation)
        transformation_rigid_matrix, rigid_mask = cv2.estimateAffinePartial2D(src_pts, dst_pts)

        # Get a mask list for matches = A list that says "This match is an in/out-lier"
        matchesMask = mask.ravel().tolist()

        # Filter the matches list thanks to the mask
        for i, element in enumerate(matchesMask):
            if element == 1:
                good.append(matches[i])

        return good, transformation_matrix, transformation_rigid_matrix

def grabImage(imgName):
    def grabInternal(imgName):
        if grabMode == 1:
            ret,frame = reader.read()
            return frame
        else:
            if isinstance(imgName, bytes):
                imgName=imgName.decode('utf-8')
            print(imgName)
            # load the image and convert it to grayscale
            image=cv2.imread(imgName)
            return image
    
    frame = grabInternal(imgName)
    if frame is None:
        return None, False, None
    greyscale = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    if shouldDiscardImage(greyscale):
        return None, True, greyscale
    else:
        return frame, False, greyscale

def run():
    if grabMode == 1:
        totalFrames = int(reader.get(cv2.CAP_PROP_FRAME_COUNT))
        imgs = [None]*totalFrames
    else:
        imgs=p.stdout.split(b'\n')
    i = 0
    img1 = None
    discarded = True # Assume True to start with
    greyscale = None
    if len(imgs) > 0:
        while img1 is None and discarded:
            img1, discarded, greyscale = grabImage(imgs[i])
            i+=1
        firstImage = img1
        
        mask2 = np.zeros_like(greyscale)
        hOrig, wOrig = img1.shape[:2]
        xc = int(wOrig / 2)
        yc = int(hOrig / 2)
        radius2 = int(hOrig * 0.45)
        test_mask=cv2.circle(mask2, (xc,yc), radius2, (255,255,255), -1) # https://stackoverflow.com/questions/42346761/opencv-python-feature-detection-how-to-provide-a-mask-sift
        cv2.imshow('test_mask', test_mask)
        cv2.waitKey(0)
    
        kp1, des1 = sift.detectAndCompute(img1,mask = test_mask) # Returns keypoints and descriptors
    else:
        print("No images")

    imgs_iter = iter(imgs)
    next(imgs_iter) # Skip first image
    #acc = np.matrix([[1,0,0],[0,1,0],[0,0,1]])
    acc = np.matrix([[1.0,0.0,0.0],[0.0,1.0,0.0],[0.0,0.0,1.0]])
    #i = 1 # (Skipped first image)
    for imgName in imgs_iter:
        def waitForInput():
            waitAmount = 1 if i < len(imgs) - 20 else 0
            if waitAmount == 0:
                print("Press a key to continue")
                if i == len(imgs) - 1:
                    print("(Last image)")
            cv2.waitKey(waitAmount)
        
        img2, discarded, greyscale = grabImage(imgName)
        if img2 is None and not discarded:
            print("img2 was None")
            cv2.waitKey(0)
            break
        if img2 is None and discarded:
            waitForInput()
            i += 1
            continue
        hOrig, wOrig = img2.shape[:2]

        # find the keypoints and descriptors with SIFT
        kp2, des2 = sift.detectAndCompute(img2,mask = test_mask) # Returns keypoints and descriptors

        # Find matches
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
        
        good, transformation_matrix, transformation_rigid_matrix = find_homography(kp2, kp1, good_flatList) # Swap order of kp1, kp2 because want to find first image in the second
        if transformation_matrix is None:
            print("transformation_matrix was None")
            cv2.waitKey(0)
            break
        acc *= transformation_matrix
        accOld = acc.copy()
        # #print(acc[0])
        # acc[0][0] *= 0.9
        # #print(acc[0])
        # acc[0][1] *= 0.9
        # acc[1][0] *= 0.9
        # acc[0][1] *= 0.9
        print("acc:", accOld, acc)

        img3 = None
        img3 = cv2.drawMatchesKnn(img1,kp1,img2,kp2,good_old if mode==0 else list(map(lambda x: [x], good)),outImg=img3,flags=2)
        cv2.imshow('matching',img3);
        tr = cv2.warpPerspective(img1, transformation_matrix, (wOrig, hOrig))
        cv2.imshow('current transformation',tr)
        tr = cv2.warpPerspective(firstImage, np.linalg.pinv(acc), (wOrig, hOrig))
        cv2.imshow('acc',tr);
        waitForInput()
        #plt.imshow(img3),plt.show()
        
        # Save current keypoints as {the prev keypoints for next iteration}
        kp1=kp2
        des1=des2
        img1=img2
        i+=1

run()
