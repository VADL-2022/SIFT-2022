import numpy as np
import cv2
import cv2 as cv
from sys import argv
import sys
from os import path
sys.path.append(path.abspath(path.join(path.dirname(__file__), "..")))
import knn_matcher2

trMask=np.matrix([[1,0,1],
                  [0,1,1],
                  [0,0,1]], dtype=np.dtype(np.uint8))

# Read the query image as query_img
# and train image This query image
# is what you need to find in train image
# Save it in the same directory
# with the name image.jpg 
query_img = cv2.imread(argv[2])
train_img = cv2.imread(argv[1])

# # Resize
image = query_img
hOrig, wOrig = image.shape[:2]
# w=int(wOrig/2.5)
# h=int(hOrig/2.5)
w=wOrig
h=hOrig
print(w,h)
query_img=cv2.resize(query_img, (w,h))
train_img=cv2.resize(train_img, (w,h))

# Convert it to grayscale
query_img_bw = cv2.cvtColor(query_img,cv2.COLOR_BGR2GRAY)
train_img_bw = np.invert(cv2.cvtColor(train_img, cv2.COLOR_BGR2GRAY))
#train_img_bw = cv2.cvtColor(train_img, cv2.COLOR_BGR2GRAY)

# Color quantization

import cv2
import numpy as np

# Kmeans color segmentation
def kmeans_color_quantization(image, clusters=8, rounds=1):
    h, w = image.shape[:2]
    samples = np.zeros([h*w,3], dtype=np.float32)
    count = 0

    for x in range(h):
        for y in range(w):
            samples[count] = image[x][y]
            count += 1

    compactness, labels, centers = cv2.kmeans(samples,
            clusters, 
            None,
            (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 10000, 0.0001), 
            rounds, 
            cv2.KMEANS_RANDOM_CENTERS)

    centers = np.uint8(centers)
    res = centers[labels.flatten()]
    return res.reshape((image.shape))

# Load image and perform kmeans
image = query_img #cv2.imread('1.jpg')
kmeans = kmeans_color_quantization(image, clusters=6)
query_img = kmeans.copy()
cv2.imshow('kmeans1', query_img)
image = train_img #cv2.imread('1.jpg')
kmeans = kmeans_color_quantization(image, clusters=6)
result = kmeans.copy()
train_img = kmeans.copy()
cv2.imshow('kmeans2', train_img)

# Convert it to grayscale
query_img_bw = cv2.cvtColor(query_img,cv2.COLOR_BGR2GRAY)
train_img_bw = np.invert(cv2.cvtColor(train_img, cv2.COLOR_BGR2GRAY))
#train_img_bw = cv2.cvtColor(train_img, cv2.COLOR_BGR2GRAY)

# # Floodfill
seed_point = (int(w/2), 50)
ret, img, mask, rect = cv2.floodFill(query_img, None, seedPoint=seed_point, newVal=(36, 255, 12), loDiff=(0, 0, 0, 0), upDiff=(0, 0, 0, 0))
cv2.imshow('floodfill1', img)
query_img=img.copy()
h2, w2 = image.shape[:2]
seed_point = (int(w2/2), int(h2/2))
ret, img, mask, rect = cv2.floodFill(train_img, None, seedPoint=seed_point, newVal=(36, 255, 12), loDiff=(0, 0, 0, 0), upDiff=(0, 0, 0, 0))
cv2.imshow('floodfill2', img)
train_img=img.copy()
cv2.waitKey()

# Convert it to grayscale
query_img_bw = cv2.cvtColor(query_img,cv2.COLOR_BGR2GRAY)
train_img_bw = (cv2.cvtColor(train_img, cv2.COLOR_BGR2GRAY))

# cv2.imshow('image', image)
# cv2.imshow('kmeans', kmeans)
# cv2.imshow('result', result)
# cv2.waitKey()     


# img=query_img_bw
# cv2.GaussianBlur(img, (11, 11), 0, img)
# img=train_img_bw
# cv2.GaussianBlur(img, (11, 11), 0, img)

counter = 0
def thresh(img, t):
    global counter
    blurred = cv2.GaussianBlur(img, (11, 11), 0)
    #blurred = cv2.GaussianBlur(greyscaleImage, (round_up_to_odd(7 * scaleX), round_up_to_odd(7 * scaleY)), 2, None, 2)
    #imageBrightnessThreshold = 200
    imageBrightnessThreshold = t
    thresh = cv2.threshold(blurred, imageBrightnessThreshold, 255, cv2.THRESH_BINARY)[1]
    #thresh = cv2.erode(thresh, None, thresh, iterations=2)
    #thresh = cv2.erode(thresh, None, thresh, iterations=4)
    #thresh = cv2.dilate(thresh, None, thresh, iterations=4)
    cv2.imshow(str(counter),thresh)
    counter += 1
    return thresh
def edges(img):
    edges = cv2.Canny(img,100,200) # params = img, lower and upper threshold
    return edges
#query_img_bw=thresh(query_img_bw, 120)
#train_img_bw=thresh(train_img_bw, 130)

#query_img_bw=edges(query_img_bw)
#train_img_bw=edges(train_img_bw)




# im=query_img_bw
# max=205
# # blockSize=31
# # C=15
# blockSize=21
# C=18
# cv.adaptiveThreshold(im, max, cv.ADAPTIVE_THRESH_MEAN_C, 
#                                   cv.THRESH_BINARY, blockSize, C, im)
# im=train_img_bw
# cv.adaptiveThreshold(im, max, cv.ADAPTIVE_THRESH_MEAN_C, 
#                                   cv.THRESH_BINARY, blockSize, C, im)

# brightness=0.5
# contrast=1
# img=query_img_bw
# query_img_bw = cv2.addWeighted( img, contrast, img, 0, brightness)
# brightness=0.5
# contrast=1
# img=train_img_bw
# train_img_bw = cv2.addWeighted( img, contrast, img, 0, brightness)

def shapes(img, imgGry, outCanvas, tmin, showText=False):
    ret, thrash = cv2.threshold(imgGry, tmin , 255, cv2.CHAIN_APPROX_NONE)
    contours , hierarchy = cv2.findContours(thrash, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE)
    #outPoints=np.zeros((len(contours),2), dtype=np.float32)
    outPoints=[None]*len(contours)
    counter=0
    for contour in contours:
        (x,y),radius = cv2.minEnclosingCircle(contour) # https://www.google.com/search?q=opencv+approxpolydp+radius&sxsrf=APq-WBsZWnuPyUHjolKGlhaoka801zMWOg%3A1649959940477&ei=BGRYYpngHJGY_Qa9g6n4Cw&ved=0ahUKEwiZ8pThk5T3AhURTN8KHb1BCr8Q4dUDCA4&uact=5&oq=opencv+approxpolydp+radius&gs_lcp=Cgdnd3Mtd2l6EAM6BwgAEEcQsAM6BwgAELADEEM6BAgAEEM6BQgAEIAEOgYIABAWEB46BQgAEIYDOgUIIRCgAUoECEEYAEoECEYYAFCTAVisBWC1BmgBcAF4AIABZIgBtQSSAQM2LjGYAQCgAQHIAQrAAQE&sclient=gws-wiz -> https://stackoverflow.com/questions/41561731/opencv-detecting-circular-shapes
        approx = cv2.approxPolyDP(contour, 0.01* cv2.arcLength(contour, True), True)
        cv2.drawContours(outCanvas, [approx], 0, (0, 0, 0), 5)
        x = approx.ravel()[0]
        y = approx.ravel()[1] - 5
        cv2.circle(outCanvas, (x,y), int(radius), (255,0,0), 2)
        if len(approx) == 3:
            if showText: cv2.putText( outCanvas, "Triangle", (x, y), cv2.FONT_HERSHEY_COMPLEX, 0.5, (0, 0, 0) )
        elif len(approx) == 4 :
            x, y , w, h = cv2.boundingRect(approx)
            aspectRatio = float(w)/h
            print(aspectRatio)
            if aspectRatio >= 0.95 and aspectRatio < 1.05:
                if showText: cv2.putText(outCanvas, "square", (x, y), cv2.FONT_HERSHEY_COMPLEX, 0.5, (0, 0, 0))

            else:
                if showText: cv2.putText(outCanvas, "rectangle", (x, y), cv2.FONT_HERSHEY_COMPLEX, 0.5, (0, 0, 0))

        elif len(approx) == 5 :
            if showText: cv2.putText(outCanvas, "pentagon", (x, y), cv2.FONT_HERSHEY_COMPLEX, 0.5, (0, 0, 0))
        elif len(approx) == 10 :
            if showText: cv2.putText(outCanvas, "star", (x, y), cv2.FONT_HERSHEY_COMPLEX, 0.5, (0, 0, 0))
        else:
            if showText: cv2.putText(outCanvas, "circle", (x, y), cv2.FONT_HERSHEY_COMPLEX, 0.5, (0, 0, 0))
        #print((x,y))
        #outPoints[counter] = cv2.KeyPoint(float(x),float(y),(radius*2)) # the diameter ("Size") is used as a descriptor ( https://answers.opencv.org/question/54119/keypoint-response-and-size-what-are-they/ , https://docs.opencv.org/4.x/d2/d29/classcv_1_1KeyPoint.html#a308006c9f963547a8cff61548ddd2ef2 )
        outPoints[counter] = (x,y)
        counter += 1
    return outCanvas, outPoints
canvas=np.ones((h,w))
cv2.imshow('query_img_bw',query_img_bw)
img,points1=shapes(query_img_bw, query_img_bw, canvas, 150)
cv2.imshow('shapes1',img)

h, w = train_img_bw.shape[:2]
canvas=np.ones((h,w))
#print(w,h)
#exit(0)
cv2.imshow('train_img_bw',train_img_bw)
img,points2=shapes(train_img_bw, train_img_bw, canvas, 150)
cv2.imshow('shapes2',img)

print(points1, '\n', points2)
if len(points1) != len(points2):
    # Hack them to be the same size
    largest=max(len(points1), len(points2))
    smallest=min(len(points1), len(points2))
    smallestArray = points2 if len(points2) == smallest else points1
    for i in range(largest-smallest):
        smallestArray.append((0,0))
points1=np.float32(points1).reshape(-1, 1, 2)
points2=np.float32(points2).reshape(-1, 1, 2)
transformation_matrix1, mask = cv2.estimateAffine2D(points2, points1, np.array([]), cv2.RANSAC, 5.0)
if transformation_matrix1 is not None:
    # https://stackoverflow.com/questions/3881453/numpy-add-row-to-array
    transformation_matrix1 = np.append(transformation_matrix1, [[0.0,0.0,1.0]], axis=0) # Make affine into perspective matrix
    print(transformation_matrix1)
if transformation_matrix1 is None:
    print("transformation_matrix1 was None")
    cv2.waitKey(0)
else:
    print("Showing lerp controller")
    #transformation_matrix1=np.linalg.pinv(transformation_matrix1)
    knn_matcher2.showLerpController(query_img, transformation_matrix1, idMat=cv2.bitwise_and(knn_matcher2.idMat, knn_matcher2.idMat, mask=trMask))

#exit(0)

#cv2.waitKey(0)
def blobDetect(img):
    blank = np.zeros((1,1))
    # https://stackoverflow.com/questions/30291958/feature-detection-with-opencv-fails-with-seg-fault #
    params = cv2.SimpleBlobDetector_Params()
    params.minArea=10
    ver = (cv2.__version__).split('.')
    if int(ver[0]) < 3 :
        detector = cv2.SimpleBlobDetector(params)
    else : 
        detector = cv2.SimpleBlobDetector_create(params)
    # #
    keypoints = detector.detect(img)
    blobs = cv2.drawKeypoints(img, keypoints, blank, (0,255,255), cv2.DRAW_MATCHES_FLAGS_DEFAULT)
    return blobs
img=query_img_bw
blobs=blobDetect(img)
cv2.imshow('blobs', blobs)
img=train_img_bw
blobs=blobDetect(img)
cv2.imshow('blobs2', blobs)

# Initialize the ORB detector algorithm
orb = cv2.ORB_create()
#orb = cv2.xfeatures2d.SIFT_create()
  
# Now detect the keypoints and compute
# the descriptors for the query image
# and train image
queryKeypoints, queryDescriptors = orb.detectAndCompute(query_img_bw,None)
trainKeypoints, trainDescriptors = orb.detectAndCompute(train_img_bw,None)

if ( len(queryKeypoints)==0 ):
   print("1st keypoints empty")
   exit(0)
if ( len(trainKeypoints)==0 ):
   print("2nd keypoints empty")
   exit(0)
if ( len(queryDescriptors)==0 ):
   print("1st descriptor empty")
   exit(0)
if ( len(trainDescriptors)==0 ):
   print("2nd descriptor empty")
   exit(0)
   
# Initialize the Matcher for matching
# the keypoints and then match the
# keypoints
matcher = cv.DescriptorMatcher_create(cv.DescriptorMatcher_FLANNBASED) #cv2.BFMatcher()
#matches = matcher.match(queryDescriptors,trainDescriptors)
matches = matcher.knnMatch(np.float32(queryDescriptors), np.float32(trainDescriptors), 2)
matches=matches[0]
  
# draw the matches to the final image
# containing both the images the drawMatches()
# function takes both images and keypoints
# and outputs the matched query image with
# its train image
#m=matches[:20]
m=matches
final_img = cv2.drawMatches(query_img_bw, queryKeypoints,
train_img_bw, trainKeypoints, m,None)
  
final_img = cv2.resize(final_img, (1000,650))
 
# Show the final image
cv2.imshow("Matches", final_img)
cv2.waitKey(0)


# Apply ratio test
good = []
good_flatList = []
for m in matches:
    #m
    if True: #if m.distance < 0.75*n.distance:
        good.append([m])
        good_flatList.append(m)

        # cv2.drawMatchesKnn expects list of lists as matches.
        #img3 = None
        #img3 = cv2.drawMatchesKnn(img1,kp1,img2,kp2,good,img3,flags=2)
good_old = good

#print("queryKeypoints:", queryKeypoints)
good, transformation_matrix, transformation_rigid_matrix = knn_matcher2.find_homography(queryKeypoints, trainKeypoints, good_flatList)
print(transformation_matrix)
if transformation_matrix is None:
    print("transformation_matrix is None")
    exit(0)
# Keep only certain aspects of the transformation
#print(transformation_matrix.dtype, trMask.dtype)
transformation_matrix = cv2.bitwise_and(transformation_matrix, transformation_matrix, mask=trMask)
print(transformation_matrix)
#tr = cv2.warpPerspective(query_img, np.linalg.pinv(transformation_matrix), (w, h))
knn_matcher2.showLerpController(query_img, transformation_matrix, idMat=cv2.bitwise_and(knn_matcher2.idMat, knn_matcher2.idMat, mask=trMask))
tr=transformation_matrix
cv2.imshow('current transformation',tr)

cv2.waitKey(0)
