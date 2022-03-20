import cv2
import numpy as np

# Camera undistortion code #

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
#mapx, mapy = cv2.fisheye.initUndistortRectifyMap(mtx, dist[:-1], None, nk, (w,h), cv2.CV_16SC2)

wUndistorted = None
hUndistorted = None
roi = None

# Configurable:
useFullImageInsteadOfROI = False

# End of undistort code #

# Helper functions copied from `../../skyDetection/brightSpotDetection2.py` (replace with new copies if needed) : #

def getCenterOfMass(thresh, # the binary image
                    ):
    #if name:
    #    cv2.imshow('crop: ' + str(name),thresh)
    #cv2.waitKey(0)
    # calculate moments of binary image
    M = cv2.moments(thresh)
    # calculate x,y coordinate of center
    try:
        cX = int(M["m10"] / M["m00"])
        cY = int(M["m01"] / M["m00"])
    except ZeroDivisionError:
        return None
    center = [cX, cY]
    return center

# Based on this C# code from https://stackoverflow.com/questions/2084695/finding-the-smallest-circle-that-encompasses-other-circles :
# public static Circle MinimalEnclosingCircle(Circle A, Circle B) {
#             double angle = Math.Atan2(B.Y - A.Y, B.X - A.X);
#             Point a = new Point((int)(B.X + Math.Cos(angle) * B.Radius), (int)(B.Y + Math.Sin(angle) * B.Radius));
#             angle += Math.PI;
#             Point b = new Point((int)(A.X + Math.Cos(angle) * A.Radius), (int)(A.Y + Math.Sin(angle) * A.Radius));
#             int rad = (int)Math.Sqrt(Math.Pow(a.X - b.X, 2) + Math.Pow(a.Y - b.Y, 2)) / 2;
#             if (rad < A.Radius) {
#                 return A;
#             } else if (rad < B.Radius) {
#                 return B;
#             } else {
#                 return new Circle((int)((a.X + b.X) / 2), (int)((a.Y + b.Y) / 2), rad);
#             }
#         }
# "Circle is defined by the X, Y of it's center and a Radius, all are ints. There's a constructor that is Circle(int X, int Y, int Radius). After breaking out some old trig concepts, I figured the best way was to find the 2 points on the circles that are farthest apart. Once I have that, the midpoint would be the center and half the distance would be the radius and thus I have enough to define a new circle. If I want to encompass 3 or more circles, I first run this on 2 circles, then I run this on the resulting encompassing circle and another circle and so on until the last circle is encompassed. There may be a more efficient way to do this, but right now it works and I'm happy with that."
import math # Import math Library
# a and b are of the following form (a list): [x, y, radius]
# NOTE: THIS FUNCTION MAY BE BUGGY. Circles from ../Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0028.png give a bad resulting circle. So to compensate, we use `bad` value < some amount as an upper bound requirement as well..
def minimalEnclosingCircle(a, b):
    angle = math.atan2(b[1] - a[1], b[0] - a[0])
    cos_angle = math.cos(angle)
    sin_angle = math.sin(angle)
    a_ = [(b[0] + cos_angle * b[2]), (b[1] + sin_angle * b[2])]
    angle += math.pi
    cos_angle = math.cos(angle)
    sin_angle = math.sin(angle)
    b_ = [(a[0] + cos_angle * a[2]), (a[1] + sin_angle * a[2])]
    rad = (math.sqrt(math.pow(a_[0] - b_[0], 2) + math.pow(a_[1] - b_[1], 2)) / 2)
    if rad < a[2]:
        return a
    elif rad < b[2]:
        return b
    else:
        return [((a_[0] + b_[0]) / 2), ((a_[1] + b_[1]) / 2), rad]

# End of copied helper functions #

def shouldDiscardImage(greyscaleImage):
    #cv2.imshow('a', greyscaleImage)
    
    #return False # temp
    blurred = cv2.GaussianBlur(greyscaleImage, (11, 11), 0)
    thresh = cv2.threshold(blurred, 200, 255, cv2.THRESH_BINARY)[1]
    thresh = cv2.erode(thresh, None, iterations=2)
    thresh = cv2.dilate(thresh, None, iterations=4)
    
    height,width=greyscaleImage.shape[:2]
    realCenter=[width / 2.0, height / 2.0]
    print("realCenter:", realCenter)
    
    # Get "center of mass" of each of the 4 quadrants of the image so we can do sky detection
    names = ['top left',
             'top right',
             'bottom left',
             'bottom right']
    tl = getCenterOfMass(thresh[0:int(height/2), 0:int(width/2)]) # top left
    tr = getCenterOfMass(thresh[0:int(height/2), int(width/2):]) # top right
    bl = getCenterOfMass(thresh[int(height/2):, 0:int(width/2)]) # bottom left
    br = getCenterOfMass(thresh[int(height/2):, int(width/2):]) # bottom right
    cms = [tl, tr, bl, br] # Centers of mass
    offsets = [[0, 0], # top left
               [int(width/2), 0], # top right
               [0, int(height/2)], # bottom left
               [int(width/2), int(height/2)] # bottom right
               ]
    bad = 0
    distanceThreshold = 0.433385 # Lower means less discards
    distanceRequired = distanceThreshold * width
    for i in range(len(cms)):
        cm = cms[i]
        name = names[i]
        if cm is None:
            print("center for", name, "was None")
            bad += 0.5
            continue
        center = np.array(cm) + np.array(offsets[i])
        print("center for", name, ":", center)
        dist_=np.linalg.norm(np.array(center) - np.array(realCenter))
        print("distance from center:", dist_)
        if dist_ < distanceRequired:
            print("  bad += 1")
            bad += 1
    # Show badness
    #<2.5 is good?
    print("badness: " + str(bad))
    
    # Show hough circle detection
    # https://docs.opencv.org/3.4/d4/d70/tutorial_hough_circle.html
    gray = cv2.GaussianBlur(greyscaleImage, (9, 9), 2, None, 2) #cv2.medianBlur(gray, 5)
    rows = gray.shape[0]
    cannyThreshold = 50 # 100
    accumulatorThreshold = 50
    minRadius = 0#int(width/2)
    maxRadius = 0#int(width)
    # Parameters documentation: https://docs.opencv.org/3.4/dd/d1a/group__imgproc__feature.html#ga47849c3be0d0406ad3ca45db65a25d2d
    circles = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT, 1, rows / 8,
                               param1=cannyThreshold, param2=accumulatorThreshold,
                               minRadius=minRadius, maxRadius=maxRadius)
                           # param1=100, param2=30,
                           # minRadius=1, maxRadius=30)
    weightedCenterOfMass = None
    radiusSum = 0
    prev = None # Becomes a "new mega hough circle"
    if circles is not None:
        weightedCenterOfMass = [0.0,0.0]
        circles = np.uint16(np.around(circles))
        # # Find largest radius
        # largestRadius=None
        # for i in circles[0, :]:
        #     radius = i[2]
        #     if largestRadius is None or radius > largestRadius:
        #         largestRadius = radius
        largestRadius=None
        for i in circles[0, :]:
            center = (i[0], i[1])
            radius = i[2]
            
            # Weighted "center of mass".. more like weighted average
            weightedCenterOfMass[0] += center[0] * radius # Weight by radius
            weightedCenterOfMass[1] += center[1] * radius
            radiusSum += radius
            if largestRadius is None or radius > largestRadius:
                largestRadius = radius

            # "New mega hough circle":
            if prev is not None:
                prev = minimalEnclosingCircle(i, prev)
            else:
                prev = i
        # Finish off the weighted average by dividing by the sum of the weights used:
        weightedCenterOfMass[0] /= radiusSum
        weightedCenterOfMass[1] /= radiusSum

        #avgRadius = radiusSum / len(circles)
        print("old mega hough circle", weightedCenterOfMass, largestRadius)
        # "New mega hough circle":
        print("mega hough circle", prev)
        # mega hough circle should be:
        # - close to center
        # - [nvm, doesn't work in later images:] smaller than image width in diameter
        # - old mega hough circle radius should be < new mega hough circle radius
    else:
        print("no hough circles")

    # Decide whether we accept this image
    megaHoughCircleDistFromCenterThreshold = 0.2
    if bad <= 3 and circles is not None and abs(prev[0] - realCenter[0]) < megaHoughCircleDistFromCenterThreshold*width and abs(prev[1] - realCenter[1]) < megaHoughCircleDistFromCenterThreshold*height:
        print("shouldDiscardImage(): GOOD IMAGE")
        return False
    else:
        print("shouldDiscardImage(): BAD IMAGE:", bad <= 3, circles is not None, abs(prev[0] - realCenter[0]) < megaHoughCircleDistFromCenterThreshold*width if prev is not None else False, abs(prev[1] - realCenter[1]) < megaHoughCircleDistFromCenterThreshold*height if prev is not None else False)
        return True


# Note: This is untested:
# def undisortImage(image):
#     global mapx
#     global mapy
#     global wUndistorted
#     global hUndistorted
#     global roi
#     hOrig, wOrig = image.shape[:2]
#     if wOrig != wUndistorted or hOrig != hUndistorted: # If image changed dimensions from our cached matrix, recompute it:
#         print("Computing new undistortion map using getOptimalNewCameraMatrix")
#         newcameramtx, roi = cv2.getOptimalNewCameraMatrix(mtx, dist, (w,h), 1, (wOrig,hOrig) # <--the "new size" ( https://docs.opencv.org/3.4/d9/d0c/group__calib3d.html#ga7a6c4e032c97f03ba747966e6ad862b1 )
#                                                           ) # Note: "getOptimalNewCameraMatrix is used to use different resolutions from the same camera with the same calibration." ( https://stackoverflow.com/questions/39432322/what-does-the-getoptimalnewcameramatrix-do-in-opencv )
#         # Notice the cv.fisheye.initUndistortRectifyMap being used instead of cv.initUndistortRectifyMap!:
#         mapx, mapy = cv2.fisheye.initUndistortRectifyMap(mtx, dist[:-1], None, newcameramtx, (wOrig,hOrig), cv2.CV_16SC2)
#         wUndistorted = wOrig
#         hUndistorted = hOrig
#     #image = cv2.resize(image, (w,h))
#     image = cv2.remap(image, mapx, mapy, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
#     #image = cv2.resize(image, (wOrig,hOrig))
    
#     if not useFullImageInsteadOfROI:
#         x, y, w_, h_ = roi
#         #print("roi:", roi)
#         image = image[y:y+h_, x:x+w_]
    
#     return image

# def undisortImage(image):
#     hOrig, wOrig = image.shape[:2]
#     image = cv2.resize(image, (w,h))
#     image = cv2.remap(image, mapx, mapy, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
#     image = cv2.resize(image, (wOrig,hOrig))
#     return image
