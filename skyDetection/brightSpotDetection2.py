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

        showContours = True
        if showContours:
            from imutils import contours
            import imutils
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
                            (0, 0, 255), 3)
                    cv2.putText(image, "#{}".format(i + 1), (x, y - 15),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 0, 255), 2)
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
