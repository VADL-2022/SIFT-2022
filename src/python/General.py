import cv2
import numpy as np
import timeit

# For preview window #
# https://docs.python.org/3/library/queue.html
import threading, queue
previewWindowQueue = queue.PriorityQueue() # https://www.educative.io/edpresso/what-is-the-python-priority-queue  # Priority based on image index (we should probably do this in matcher's queue too...)

def drainPreviewWindowQueue():
    if not previewWindowQueue.empty():
        item = previewWindowQueue.get()
        print(f'drainPreviewWindowQueue(): Starting {item}')
        item[1]()
        print(f'drainPreviewWindowQueue(): Finished {item}')
        previewWindowQueue.task_done()
# #

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
#nk[0,0]=k[0,0]/2
#nk[1,1]=k[1,1]/2
mapx = None
mapy = None

w, h = (2592, 1944)
#h, w = image.shape[:2]
newcameramtx, roi = cv2.getOptimalNewCameraMatrix(mtx, dist, (w,h), 1, (w,h))
mapx, mapy = cv2.initUndistortRectifyMap(mtx, dist, None, newcameramtx, (w,h), 5)
# Notice the cv.fisheye.initUndistortRectifyMap being used instead of cv.initUndistortRectifyMap!:
#mapx, mapy = cv2.fisheye.initUndistortRectifyMap(mtx, dist[:-1], None, nk, (w,h), cv2.CV_16SC2)

wUndistorted = None
hUndistorted = None
roi = None

# Configurable:
useFullImageInsteadOfROI = False

# End of undistort code #

# https://stackoverflow.com/questions/31648729/round-a-float-up-to-next-odd-integer
def round_up_to_odd(f):
    return int(np.ceil(f)) // 2 * 2 + 1

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
    a=np.array(a, dtype=np.float64) # To fix "RuntimeWarning: overflow encountered in ushort_scalars" ( https://stackoverflow.com/questions/7559595/python-runtimewarning-overflow-encountered-in-long-scalars )
    b=np.array(b, dtype=np.float64)
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

videoWriter = None

def shouldDiscardImage(greyscaleImage, imageIndex, shouldRunSkyDetection=True, showPreviewWindow=False, siftVideoOutputPath=None #False #Should be None but had to do a hack
                       , siftVideoOutputFPS=None):
    if not shouldRunSkyDetection:
        return False
    
    global videoWriter
    height,width=greyscaleImage.shape[:2]
    #print("BBBBBBBB",type(siftVideoOutputPath), siftVideoOutputPath)
    if siftVideoOutputPath is not None and isinstance(siftVideoOutputPath, str): #if not(isinstance(siftVideoOutputPath, bool) and siftVideoOutputPath == False): # Note: `if siftVideoOutputPath is not None:` and `if siftVideoOutputPath:` doesn't work if used here.. it thinks it is not None even though it is, maybe because it somehow has type 'str' printed in the print statement above. so we do a hack here.
        if videoWriter is None:
           # https://stackoverflow.com/questions/30103077/what-is-the-codec-for-mp4-videos-in-python-opencv
            format=cv2.VideoWriter_fourcc(*'h264')#cv2.VideoWriter_fourcc('X', 'V', 'I', 'D')
            fps=siftVideoOutputFPS
            print("Python opening video writer with params:", siftVideoOutputPath, format, fps, (width * 2, height), True)
            videoWriter = cv2.VideoWriter(siftVideoOutputPath, format, fps, (width * 2, height), True # True for writing color frames
                                          )
    #cv2.imshow('a', greyscaleImage)
    if showPreviewWindow:
        channels = greyscaleImage.shape[-1] if greyscaleImage.ndim == 3 else 1 # https://stackoverflow.com/questions/19062875/how-to-get-the-number-of-channels-from-an-image-in-opencv-2
        isGreyscale = channels == 1      # NVM: len(greyscaleImage.shape)<2 # NVM: `len(greyscaleImage.shape)<2` is true if the image is greyscale
        if isGreyscale:
            image = cv2.cvtColor(greyscaleImage, cv2.COLOR_GRAY2BGR)
        else:
            image = greyscaleImage
            greyscaleImage = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Shrink for speed up
    scaleX = 0.5
    #scaleX = 1
    scaleY = scaleX
    scaledWidth = int(width * scaleX)
    scaledHeight = int(height * scaleY)
    greyscaleImage = cv2.resize(greyscaleImage, (scaledWidth, scaledHeight))
    
    #return False # temp
    blurred = cv2.GaussianBlur(greyscaleImage, (round_up_to_odd(11 * scaleX), round_up_to_odd(11 * scaleY)), 0)
    #blurred = cv2.GaussianBlur(greyscaleImage, (round_up_to_odd(7 * scaleX), round_up_to_odd(7 * scaleY)), 2, None, 2)
    #imageBrightnessThreshold = 200
    imageBrightnessThreshold = 180
    thresh = cv2.threshold(blurred, imageBrightnessThreshold, 255, cv2.THRESH_BINARY)[1]
    #thresh = cv2.erode(thresh, None, thresh, iterations=2)
    thresh = cv2.erode(thresh, None, thresh, iterations=4)
    thresh = cv2.dilate(thresh, None, thresh, iterations=4)
    
    realCenter=[width / 2.0, height / 2.0]
    print("realCenter:", realCenter)
    if showPreviewWindow:
        cv2.circle(image, (int(realCenter[0]), int(realCenter[1])), int(2),
                   (255, 0, 0), 2)
    
    # Get "center of mass" of each of the 4 quadrants of the image so we can do sky detection
    names = ['top left',
             'top right',
             'bottom left',
             'bottom right']
    tl = getCenterOfMass(thresh[0:int(scaledHeight/2), 0:int(scaledWidth/2)]) # top left
    tr = getCenterOfMass(thresh[0:int(scaledHeight/2), int(scaledWidth/2):]) # top right
    bl = getCenterOfMass(thresh[int(scaledHeight/2):, 0:int(scaledWidth/2)]) # bottom left
    br = getCenterOfMass(thresh[int(scaledHeight/2):, int(scaledWidth/2):]) # bottom right
    cms = [tl, tr, bl, br] # Centers of mass
    offsets = [[0, 0], # top left
               [int(width/2), 0], # top right
               [0, int(height/2)], # bottom left
               [int(width/2), int(height/2)] # bottom right
               ]
    bad = 0
    distanceThreshold = 0.433385 # Lower means less discards
    distanceRequired = distanceThreshold * width
    noneCenters = 0
    badIncs = 0
    for i in range(len(cms)):
        cm = cms[i]
        name = names[i]
        if cm is None:
            print("center for", name, "was None")
            noneCenters += 1
            continue
        center = np.array(cm) / np.array([scaleX, scaleY]) + np.array(offsets[i])
        print("center for", name, ":", center)
        dist_=np.linalg.norm(np.array(center) - np.array(realCenter))
        print("distance from center:", dist_)
        if dist_ < distanceRequired:
            diff = distanceRequired - dist_
            #factor = 10
            factor = 0.1
            badInc = (diff * diff) / width * factor
            print("  bad +=", badInc)
            badIncs += 1
            bad += badInc
        if showPreviewWindow:
            cv2.circle(image, (int(center[0]), int(center[1])), int(2),
                       (0, 0, 255), 4)
            cv2.putText(image, "CM: " + name, (int(center[0]), int(center[1]) - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 0, 255), 1)
            cv2.line(image, (int(realCenter[0]), int(realCenter[1])), (int(center[0]), int(center[1])), (255, 0, 0), 2)
    if noneCenters >= 4:
        # All were bad, so no sky detected. This is ok at lower altitudes, so compensate by *subtracting* from badness:
        badInc = 2
        print("noneCenters:\n  bad -=", badInc)
        bad -= badInc
    elif noneCenters > 0:
        # badInc = (noneCenters*2) / (badIncs + 1)
        # print("noneCenters:\n  bad +=", badInc)
        # bad += badInc
        bad += 1
    if showPreviewWindow:
        cv2.putText(image, "badness: " + str(bad), (int(30), int(30)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255), 2)

        # Show min enclosing circle (it is a circle of minimum size that encloses all white pixels (except we only do one white pixel object/cluster/group: the largest one (sorted by contourArea manually below))
        # https://stackoverflow.com/questions/59099931/how-to-find-different-centers-of-various-arcs-in-a-almost-circular-hole-using-op -> https://docs.opencv.org/2.4/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html?highlight=minenclosingcircle#minenclosingcircle
        # "Next we find the circumcircle of an object using the function cv.minEnclosingCircle(). It is a circle which completely covers the object with minimum area." ( https://docs.opencv.org/3.4/dd/d49/tutorial_py_contour_features.html )
        cnts = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        cnts = cnts[0] if len(cnts) == 2 else cnts[1]
        cnts = sorted(cnts, key=cv2.contourArea, reverse=True)
        if len(cnts) > 0:
            center, radius = cv2.minEnclosingCircle(cnts[0])
            cv2.circle(image, (int(center[0]), int(center[1])), int(radius),
                       (0, 0, 255), 4)
            cv2.putText(image, "minEnclosingCircle", (int(center[0]), int(center[1]) - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 0, 255), 2)

    # Show hough circle detection
    # https://docs.opencv.org/3.4/d4/d70/tutorial_hough_circle.html
    #gray = cv2.GaussianBlur(greyscaleImage, (round_up_to_odd(9 * scaleX), round_up_to_odd(9 * scaleY)), 2, None, 2) #cv2.medianBlur(gray, 5)
    gray = cv2.GaussianBlur(greyscaleImage, (round_up_to_odd(7 * scaleX), round_up_to_odd(7 * scaleY)), 2, None, 2)
    #gray = blurred
    rows = gray.shape[0]
    #cannyThreshold = 50 # 100
    #cannyThreshold = 70
    cannyThreshold = 30
    #accumulatorThreshold = 50
    accumulatorThreshold = 70
    minRadius = 0#int(width/2)
    maxRadius = 0#int(width)
    circles = None
    def circleFinder():
        nonlocal circles
        # Parameters documentation: https://docs.opencv.org/3.4/dd/d1a/group__imgproc__feature.html#ga47849c3be0d0406ad3ca45db65a25d2d
        minDistBetweenCircles = rows / 8
        circles = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT, 1, minDistBetweenCircles,
                                   param1=cannyThreshold, param2=accumulatorThreshold,
                                   minRadius=minRadius, maxRadius=maxRadius)
                               # param1=100, param2=30,
                               # minRadius=1, maxRadius=30)
    print("cv2.HoughCircles took", timeit.timeit(circleFinder, number=1) * 1000, "milliseconds")
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
        circles_ = circles[0, :]
        print("Number of Hough circles:", len(circles_))
        for i in circles_:
            i = (i[0] / scaleX, i[1] / scaleY, i[2] / scaleX)
            center = (int(i[0]), int(i[1]))
            radius = int(i[2])

            if showPreviewWindow:
                # circle center
                cv2.circle(image, center, 1, (0, 100, 100), 1)#3)
                cv2.putText(image, "hough circle", (int(center[0]), int(center[1]) - 15),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.35,#0.95,
                            (255, 0, 255), 1)#3)
                # circle outline
                cv2.circle(image, center, radius, (255, 0, 255), 1)#3)
            
            # Weighted "center of mass".. more like weighted average.
            # `float()` is used to fix "RuntimeWarning: overflow encountered in ushort_scalars"
            weightedCenterOfMass[0] += float(center[0]) * radius # Weight by radius
            weightedCenterOfMass[1] += float(center[1]) * radius
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

        if showPreviewWindow:
            cv2.putText(image, "old mega hough circle", (int(weightedCenterOfMass[0]), int(weightedCenterOfMass[1]) - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.55,#0.95,
                        (255, 255, 255), 2)#3)
            cv2.circle(image, list(map(int,weightedCenterOfMass)), largestRadius, (255, 255, 255), 2)#3)
            # "New mega hough circle":
            cv2.putText(image, "mega hough circle", (int(prev[0]), int(prev[1]) - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.95,
                        (255, 0, 255), 3)
            cv2.circle(image, list(map(int,prev[:2])), 3, (255, 0, 255), 5)
            cv2.circle(image, list(map(int,prev[:2])), int(prev[2]), (255, 0, 255), 3)

        #avgRadius = radiusSum / len(circles)
        print("old mega hough circle", weightedCenterOfMass, largestRadius)
        # "New mega hough circle":
        print("mega hough circle", prev)
        # mega hough circle should be:
        # - close to center
        # - [nvm, doesn't work in later images:] smaller than image width in diameter
        # - old mega hough circle radius should be < new mega hough circle radius
        
        badInc = np.mean([abs(prev[0] - realCenter[0]), abs(prev[1] - realCenter[1])]) / 100
        badInc *= badInc * badInc * badInc
        print("megaHoughCircleDistFromCenter:\n  bad +=", badInc)
        bad += badInc
    else:
        print("no hough circles")
        prev = realCenter
        
    # Show badness
    #<2.5 is good?
    print("badness: " + str(bad))
        
    # Decide whether we accept this image
    megaHoughCircleDistFromCenterThreshold = 0.2
    #megaHoughCircleDistFromCenterThreshold = 0.1
    #thresholdForBad = 3
    #thresholdForBad = 2.5
    thresholdForBad = 2.075
    if (bad <= thresholdForBad and #circles is not None and
        abs(prev[0] - realCenter[0]) < megaHoughCircleDistFromCenterThreshold*width and abs(prev[1] - realCenter[1]) < megaHoughCircleDistFromCenterThreshold*height):
        print("shouldDiscardImage(): GOOD IMAGE")
        retval = False
    else:
        print("shouldDiscardImage(): BAD IMAGE:", bad <= thresholdForBad, circles is not None, abs(prev[0] - realCenter[0]) < megaHoughCircleDistFromCenterThreshold*width if prev is not None else False, abs(prev[1] - realCenter[1]) < megaHoughCircleDistFromCenterThreshold*height if prev is not None else False)
        retval = True
    if showPreviewWindow:
        cv2.putText(image, "bad image" if retval else "good image", (int(30), int(60)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255) if retval else (0, 255, 0), 2)
        def show():
            cv2.imshow("Sky detection", image)
            cv2.imshow("Sky detection mask", thresh)
        previewWindowQueue.put((imageIndex, show))
    if videoWriter is not None:
        # https://answers.opencv.org/question/202376/can-anyone-know-the-code-of-python-to-put-two-frames-in-a-single-window-output-specifically-to-use-it-in-opencv/
        concatenated = np.hstack((image, cv2.cvtColor(thresh, cv2.COLOR_GRAY2BGR))) # Put the two images side by side
        videoWriter.write(concatenated)
    return retval

def saveVideoWriter():
    videoWriter.release()

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

def undisortImage(image):
    hOrig, wOrig = image.shape[:2]
    image = cv2.resize(image, (w,h))
    image = cv2.remap(image, mapx, mapy, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
    if not useFullImageInsteadOfROI:
        roiInsetX = 500
        roiInsetY = int(roiInsetX * 0.6)
        roi=(int(roiInsetX*1.5), 129+roiInsetY, 2591-roiInsetX*3, 1717-roiInsetY*2)
        x, y, w_, h_ = roi
        image = image[y:y+h_, x:x+w_]

        start_point = (x,y)
        end_point = (x+w_,y+h_)
        color = (0, 0, 0)
        thickness = 10
        #image = cv2.rectangle(image, start_point, end_point, color, thickness)
        
        # cv2.imshow('IMAGE',image)
        # cv2.waitKey(0)
    image = cv2.resize(image, (wOrig,hOrig))
    return image
