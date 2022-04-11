import cv2
import numpy as np

imgOrig = cv2.imread("$filenameWithExtension") # FIXME: quote the filename's double quotes if any (although rare)
Mcurrent = np.linalg.inv($matrix) # Get inverse of it

def getTransformedPoint(p, matrix):
    # https://stackoverflow.com/questions/57399915/how-do-i-determine-the-locations-of-the-points-after-perspective-transform-in-t
    px = (matrix[0][0]*p[0] + matrix[0][1]*p[1] + matrix[0][2]) / ((matrix[2][0]*p[0] + matrix[2][1]*p[1] + matrix[2][2]))
    py = (matrix[1][0]*p[0] + matrix[1][1]*p[1] + matrix[1][2]) / ((matrix[2][0]*p[0] + matrix[2][1]*p[1] + matrix[2][2]))
    return px, py

size = imgOrig.shape[:2]
landingRect = [None,None,None,None]
landingRect[0] = getTransformedPoint((0, 0), Mcurrent)
landingRect[1] = getTransformedPoint((size[0], 0), Mcurrent)
landingRect[2] = getTransformedPoint((size[0], size[1]), Mcurrent)
landingRect[3] = getTransformedPoint((0, size[1]), Mcurrent)
print("landingRect:", landingRect)
