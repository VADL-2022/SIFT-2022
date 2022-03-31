# Based on https://opencv24-python-tutorials.readthedocs.io/en/latest/py_tutorials/py_feature2d/py_matcher/py_matcher.html

from __future__ import annotations
import numpy as np
import cv2
from matplotlib import pyplot as plt

#img1 = cv2.imread('box.png',0)          # queryImage
#img2 = cv2.imread('box_in_scene.png',0) # trainImage

# Initiate SIFT detector
sift = cv2.xfeatures2d.SIFT_create() # https://stackoverflow.com/questions/63949074/cv2-sift-causes-segmentation-fault  #cv2.SIFT()

# BFMatcher with default params
bf = cv2.BFMatcher()

# Config #
mode = 1
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

grabMode=1 # 1 for video, 0 for photos
reader=cv2.VideoCapture(
    # Good drone tests:
    '/Users/sebastianbond/Desktop/SeniorSemester2/RocketTeam/DroneTest_3-28-2022/2022-03-28_18_46_26_CDT/output.mp4'
    #'/Users/sebastianbond/Desktop/SeniorSemester2/RocketTeam/DroneTest_3-28-2022/2022-03-28_18_39_25_CDT/output.mp4'
)
def grabImage(imgName):
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

def run():
    import subprocess
    #p=subprocess.run(["../compareNatSort/compareNatSort", "../Data/fullscale1/Derived/SIFT/ExtractedFrames", ".png"], capture_output=True)
    if grabMode == 1:
        totalFrames = int(reader.get(cv2.CAP_PROP_FRAME_COUNT))
        imgs = [None]*totalFrames
    else:
        p=subprocess.run(["compareNatSort/compareNatSort",
                          "Data/fullscale1/Derived/SIFT/ExtractedFrames"
                          # '/Users/sebastianbond/Desktop/sdCardImages/sdCardN64RaspberryPiImage/2022-03-21_19_02_14_CDT'
                          #'/Users/sebastianbond/Desktop/sdCardImages/sdCardN64RaspberryPiImage/2022-03-21_18_47_47_CDT'
                          , ".png"], capture_output=True)
        imgs=p.stdout.split(b'\n')
    if len(imgs) > 0:
        img1 = grabImage(imgs[0])
        firstImage = img1
        kp1, des1 = sift.detectAndCompute(img1,None) # Returns keypoints and descriptors
    else:
        print("No images")

    imgs_iter = iter(imgs)
    next(imgs_iter) # Skip first image
    #acc = np.matrix([[1,0,0],[0,1,0],[0,0,1]])
    acc = np.matrix([[1.0,0.0,0.0],[0.0,1.0,0.0],[0.0,0.0,1.0]])
    i = 1 # (Skipped first image)
    for imgName in imgs_iter:
        img2 = grabImage(imgName)
        if img2 is None:
            print("img2 was None")
            cv2.waitKey(0)
            break
        hOrig, wOrig = img2.shape[:2]

        # find the keypoints and descriptors with SIFT
        kp2, des2 = sift.detectAndCompute(img2,None) # Returns keypoints and descriptors

        # Find matches
        matches = bf.knnMatch(des1,des2, k=2)

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
        
        good, transformation_matrix, transformation_rigid_matrix = find_homography(kp1, kp2, good_flatList)
        acc *= transformation_matrix
        print(acc)

        img3 = None
        img3 = cv2.drawMatchesKnn(img1,kp1,img2,kp2,good_old if mode==0 else list(map(lambda x: [x], good)),outImg=img3,flags=2)
        cv2.imshow('matching',img3);
        tr = cv2.warpPerspective(img1, transformation_matrix, (wOrig, hOrig))
        cv2.imshow('current transformation',tr)
        tr = cv2.warpPerspective(firstImage, acc, (wOrig, hOrig))
        cv2.imshow('acc',tr);
        waitAmount = 1 if i < len(imgs) - 5 else 0
        if waitAmount == 0:
            print("Press a key to continue")
            if i == len(imgs) - 1:
                print("(Last image)")
        cv2.waitKey(waitAmount)
        #plt.imshow(img3),plt.show()
        
        # Save current keypoints as {the prev keypoints for next iteration}
        kp1=kp2
        des1=des2
        img1=img2
        i+=1

run()
