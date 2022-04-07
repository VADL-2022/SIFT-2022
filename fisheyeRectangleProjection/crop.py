
from __future__ import annotations
import numpy as np
import cv2
from matplotlib import pyplot as plt
import os
from pathlib import Path
import sys
from datetime import datetime

frame = cv2.imread('../Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0035.png')
greyscale = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
mask2 = np.zeros_like(greyscale)
hOrig, wOrig = frame.shape[:2]
xc = int(wOrig / 2)
yc = int(hOrig / 2)
radius2 = int(hOrig * 0.45)
test_mask=cv2.circle(mask2, (xc,yc), radius2, (255,255,255), -1)
m_thresh_color = cv2.bitwise_and(frame, frame, mask=test_mask)
cv2.imwrite('threshed.png', m_thresh_color)
