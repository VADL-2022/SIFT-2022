# Demo: python3 brightSpotDetection.py -i ../Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0001.png --radius 51
# Modified from https://pyimagesearch.com/2014/09/29/finding-brightest-spot-image-using-python-opencv/

# import the necessary packages
import numpy as np
import argparse
import cv2
from skimage import measure
# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-i", "--image", help = "path to the image file",default=None)
ap.add_argument("-u", "--undistort", help = "whether to undistort the image", default=False, type=bool)
ap.add_argument("-r", "--radius", type = int,
	        help = "radius of Gaussian blur; must be odd", default=51)
args = vars(ap.parse_args())

# The calibration data #
mtx = np.matrix([[7.35329235e+02, 0.00000000e+00, 1.28117269e+03],
                [0.00000000e+00, 7.40147960e+02, 9.90415290e+02],
                [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist = np.matrix([[-0.25496947,  0.07321429, -0.00104747,  0.00103111, -0.00959384]])
# #

askedOnce = False

def getCenterOfMass(thresh, # the binary image
                    name):
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

def run():
    global askedOnce
    
    k=mtx
    # https://stackoverflow.com/questions/34316306/opencv-fisheye-calibration-cuts-too-much-of-the-resulting-image
    nk = k.copy()
    nk[0,0]=k[0,0]/2
    nk[1,1]=k[1,1]/2
    mapx = None
    mapy = None
    
    if args["image"] is None:
        import subprocess
        p=subprocess.run(["../compareNatSort/compareNatSort", "../Data/fullscale1/Derived/SIFT/ExtractedFrames", ".png"], capture_output=True)
        imgs=p.stdout.split(b'\n')
    else:
        imgs=[args["image"]] # i.e., `multi_bright_regions_input_01.jpg`

    for imgName in imgs:
        if isinstance(imgName, bytes):
            imgName=imgName.decode('utf-8')
        print(imgName)
        # load the image and convert it to grayscale
        image=cv2.imread(imgName)
        hOrig, wOrig = image.shape[:2]
        cv2.imshow("Original", image)
        if args["undistort"]:
            image = cv2.resize(image, (2592, 1944))

        if mapx is None:
            h, w = image.shape[:2]
            # Notice the cv.fisheye.initUndistortRectifyMap being used instead of cv.initUndistortRectifyMap!:
            mapx, mapy = cv2.fisheye.initUndistortRectifyMap(mtx, dist[:-1], None, nk, (w,h), cv2.CV_16SC2)
        if args["undistort"]:
            image = cv2.remap(image, mapx, mapy, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
        
        #image = cv2.imread(args["image"])
        orig = image.copy()
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        # perform a naive attempt to find the (x, y) coordinates of
        # the area of the image with the largest intensity value
        (minVal, maxVal, minLoc, maxLoc) = cv2.minMaxLoc(gray)
        cv2.circle(image, maxLoc, 5, (255, 0, 0), 2)
        # display the results of the naive attempt
        cv2.imshow("Naive", image)
        # apply a Gaussian blur to the image then find the brightest
        # region
        gray = cv2.GaussianBlur(gray, (args["radius"], args["radius"]), 0)
        (minVal, maxVal, minLoc, maxLoc) = cv2.minMaxLoc(gray)
        image = orig.copy()
        cv2.circle(image, maxLoc, args["radius"], (255, 0, 0), 2)
        # display the results of our newly improved method
        cv2.imshow("Robust", image)
        #cv2.waitKey(0)

        # https://pyimagesearch.com/2016/10/31/detecting-multiple-bright-spots-in-an-image-with-python-and-opencv/ #
        # load the image, convert it to grayscale, and blur it
        image = orig.copy()
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        blurred = cv2.GaussianBlur(gray, (11, 11), 0)
        
        # threshold the image to reveal light regions in the
        # blurred image
        thresh = cv2.threshold(blurred, 200, 255, cv2.THRESH_BINARY)[1]

        # perform a series of erosions and dilations to remove
        # any small blobs of noise from the thresholded image
        #thresh = cv2.erode(thresh, None, iterations=15)
        thresh = cv2.erode(thresh, None, iterations=2)
        thresh = cv2.dilate(thresh, None, iterations=4)

        # perform a connected component analysis on the thresholded
        # image, then initialize a mask to store only the "large"
        # components
        labels = measure.label(thresh, connectivity=2, background=0) #labels = measure.label(thresh, neighbors=8, background=0) <-- neighbors= is deprecated, use connectivity instead: https://github.com/scikit-image/scikit-image/issues/4020
        mask = np.zeros(thresh.shape, dtype="uint8")
        # loop over the unique components
        for label in np.unique(labels):
                # if this is the background label, ignore it
                if label == 0:
                        continue
                # otherwise, construct the label mask and count the
                # number of pixels 
                labelMask = np.zeros(thresh.shape, dtype="uint8")
                labelMask[labels == label] = 255
                numPixels = cv2.countNonZero(labelMask)
                # if the number of pixels in the component is sufficiently
                # large, then add it to our mask of "large blobs"
                if numPixels > 300:
                        mask = cv2.add(mask, labelMask)

        # Find "center of mass" of the binary image:
        # https://stackoverflow.com/questions/65304623/calculating-the-center-of-mass-of-a-binary-image-opencv-python
        # Idea: as it gets *closer* to the center, worse orientation (not looking down).
        #center = [ np.average(indices) for indices in np.where(mask >= 255) ] # x and y
        # # Same as the above:
        # # Find indices where we have mass
        # mass_x, mass_y = np.where(mask >= 255)
        # # mass_x and mass_y are the list of x indices and y indices of mass pixels

        # cent_x = np.average(mass_x)
        # cent_y = np.average(mass_y)
        # center = [cent_x, cent_y]
        
        height,width=image.shape[:2]
        realCenter=[width / 2.0, height / 2.0]
        print("realCenter:", realCenter)
        cv2.circle(image, (int(realCenter[0]), int(realCenter[1])), int(2),
                   (255, 0, 0), 2)
        
        # Get "center of mass" of each of the 4 quadrants of the image so we can do sky detection
        # Demo of cropping: https://stackoverflow.com/questions/15589517/how-to-crop-an-image-in-opencv-using-python :
        #     top=514
        #     right=430
        #     height= 40
        #     width=100
        #     croped_image = image[top : (top + height) , right: (right + width)]
        #     plt.imshow(croped_image, cmap="gray")
        #     plt.show()
        names = ['top left',
                 'top right',
                 'bottom left',
                 'bottom right']
        tl = getCenterOfMass(thresh[0:int(height/2), 0:int(width/2)], names[0]) # top left
        tr = getCenterOfMass(thresh[0:int(height/2), int(width/2):], names[1]) # top right
        bl = getCenterOfMass(thresh[int(height/2):, 0:int(width/2)], names[2]) # bottom left
        br = getCenterOfMass(thresh[int(height/2):, int(width/2):], names[3]) # bottom right
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
            cv2.circle(image, (int(center[0]), int(center[1])), int(2),
                       (0, 0, 255), 4)
            cv2.putText(image, "CM: " + name, (int(center[0]), int(center[1]) - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 0, 255), 1)
            cv2.line(image, (int(realCenter[0]), int(realCenter[1])), (int(center[0]), int(center[1])), (255, 0, 0), 2)
        # Show badness
        #<2.5 is good?
        cv2.putText(image, "badness: " + str(bad), (int(30), int(30)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255), 2)

        # Show min enclosing circle (it is a circle of minimum size that encloses all white pixels (except we only do one white pixel object/cluster/group: the largest one (sorted by contourArea manually below))
        # https://stackoverflow.com/questions/59099931/how-to-find-different-centers-of-various-arcs-in-a-almost-circular-hole-using-op -> https://docs.opencv.org/2.4/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html?highlight=minenclosingcircle#minenclosingcircle
        # "Next we find the circumcircle of an object using the function cv.minEnclosingCircle(). It is a circle which completely covers the object with minimum area." ( https://docs.opencv.org/3.4/dd/d49/tutorial_py_contour_features.html )
        cnts = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        cnts = cnts[0] if len(cnts) == 2 else cnts[1]
        cnts = sorted(cnts, key=cv2.contourArea, reverse=True)
        center, radius = cv2.minEnclosingCircle(cnts[0])
        cv2.circle(image, (int(center[0]), int(center[1])), int(radius),
                   (0, 0, 255), 4)
        cv2.putText(image, "minEnclosingCircle", (int(center[0]), int(center[1]) - 15),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 0, 255), 2)

        # Show hough circle detection
        # https://docs.opencv.org/3.4/d4/d70/tutorial_hough_circle.html
        gray = cv2.cvtColor(orig, cv2.COLOR_BGR2GRAY)
        # GaussianBlur documentation: https://docs.opencv.org/4.x/d4/d86/group__imgproc__filter.html#gaabe8c836e97159a9193fb0b11ac52cf1
        gray = cv2.GaussianBlur(gray, (9, 9), 2, None, 2) #cv2.medianBlur(gray, 5)
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
                # circle center
                cv2.circle(image, center, 1, (0, 100, 100), 1)#3)
                cv2.putText(image, "hough circle", (int(center[0]), int(center[1]) - 15),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.35,#0.95,
                            (255, 0, 255), 1)#3)
                # circle outline
                radius = i[2]
                cv2.circle(image, center, radius, (255, 0, 255), 1)#3)

                # Weighted "center of mass".. more like weighted average
                weightedCenterOfMass[0] += center[0] * radius # Weight by radius
                weightedCenterOfMass[1] += center[1] * radius
                radiusSum += radius
                if largestRadius is None or radius > largestRadius:
                    largestRadius = radius
            # Finish off the weighted average by dividing by the sum of the weights used:
            weightedCenterOfMass[0] /= radiusSum
            weightedCenterOfMass[1] /= radiusSum

            #avgRadius = radiusSum / len(circles)
            cv2.putText(image, "mega hough circle", (int(weightedCenterOfMass[0]), int(weightedCenterOfMass[1]) - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.95,
                        (255, 0, 255), 3)
            cv2.circle(image, list(map(int,weightedCenterOfMass)), largestRadius, (255, 0, 255), 3)
        else:
            print("no hough circles")
        
        showContours = True
        if showContours:
            from imutils import contours
            import imutils
            try:
                # find the contours in the mask, then sort them from left to
                # right
                cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,
                        cv2.CHAIN_APPROX_SIMPLE)
                cnts = imutils.grab_contours(cnts)
                cnts = contours.sort_contours(cnts)[0]
                # loop over the contours
                for (i, c) in enumerate(cnts):
                        # draw the bright spot on the image
                        (x, y, w, h) = cv2.boundingRect(c)
                        ((cX, cY), radius) = cv2.minEnclosingCircle(c)
                        cv2.circle(image, (int(cX), int(cY)), int(radius),
                                   (0, 0, 255), 1)
                        cv2.circle(image, (int(cX), int(cY)), int(1),
                                   (0, 0, 255), 2)
                        cv2.putText(image, "#{}".format(i + 1), (x, y - 15),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 0, 255), 1)
            except ValueError:
                pass
        # show the output image
        cv2.imshow("Multiple bright spots: mask",mask)
        cv2.imshow("Multiple bright spots", image)
        #cv2.waitKey(0)
        # #

        key=cv2.waitKey(0)
        if key & 0xFF == ord('q'): # https://stackoverflow.com/questions/20539497/python-opencv-waitkey0-does-not-respond
            exit(0)
        elif key & 0xFF == ord('w'): # Write
            val = input("Overwrite image \"" + imgName + "\"? (y/n): ") if not askedOnce else 'y'
            if val == 'y' or val == "Y":
                askedOnce = True
                # Write/save the calibration (overwrite image)
                orig = cv2.resize(orig, (wOrig, hOrig))
                cv2.imwrite(imgName, orig)
            # Go to next image
            continue
        elif key & 0xFF == ord('s'): # Skip
            # Go to next image
            continue


run()
