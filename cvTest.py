import cv2
import time
from os import path

cap = cv2.VideoCapture(-1) #0 or -1
if cap.isOpened():
    ret, img = cap.read()
    if ret:
        #cv2.imwrite('sample.jpg', img)
        cv2.imshow('sample',img)
        
        #waits for user to press any key 
        #(this is necessary to avoid Python kernel form crashing)
        cv2.waitKey(0)
    else:
        print("error")

else:
    print('no camera!')
cap.release()
cv2.destroyAllWindows() 
