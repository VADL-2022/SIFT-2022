import cv2
import numpy as np

# M = np.array([2.939, -0.6294, -224.8,
#               0.7125, 1.505, -322.5,
#               0.003311, -0.004268, 1.239])
M = np.matrix([[2.939, -0.6294, -224.8],
              [0.7125, 1.505, -322.5],
              [0.003311, -0.004268, 1.239]])
img = cv2.imread("dataOutput/firstImage8.png")
#cv2.warpAffine(img, img, M, img.size())
cv2.warpPerspective(img, img, M, img.size())
cv2.imshow(img)

img = cv2.imread("dataOutput/scaled46.png")
cv2.warpPerspective(img, img, M.inv(), img.size())
cv2.imshow(img)
