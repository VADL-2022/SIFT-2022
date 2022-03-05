import numpy as np
import cv2

# https://stackoverflow.com/questions/9041681/opencv-python-rotate-image-by-x-degrees-around-specific-point
# Exemple with img center point:
# angle = np.pi/6
# specific_point = np.array(img.shape[:2][::-1])/2
def rotate(img: np.ndarray, angle: float, specific_point: np.ndarray) -> np.ndarray:
    warp_mat = np.zeros((2,3))
    cos, sin = np.cos(angle), np.sin(angle)
    warp_mat[:2,:2] = [[cos, -sin],[sin, cos]]
    warp_mat[:2,2] = specific_point - np.matmul(warp_mat[:2,:2], specific_point)
    return cv2.warpAffine(img, warp_mat, img.shape[:2][::-1])

def run():
    import subprocess
    p=subprocess.run(["compareNatSort/compareNatSort", "Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated", ".png"], capture_output=True)
    imgs=p.stdout.split(b'\n')
    angle = np.rad2deg(np.pi/6)
    print("Output is image followed by its angle used:")
    for imgName in imgs:
        imgName=imgName.decode('utf-8')
        print(imgName)
        img=cv2.imread(imgName)
        #print(img)
        img2=None

        specific_point = np.array(img.shape[:2][::-1])/2
        windowName='undistort test'
        def on_change(val):
            nonlocal img2
            nonlocal angle
            angle=val
            img2=rotate(img, np.deg2rad(angle), specific_point)
            cv2.imshow(windowName,img2)
        on_change(angle)
        cv2.createTrackbar('slider', windowName, 0, 360, on_change)
        key=cv2.waitKey(0)
        if key & 0xFF == ord('q'): # https://stackoverflow.com/questions/20539497/python-opencv-waitkey0-does-not-respond
            exit(0)
        elif key & 0xFF == ord('w'): # Write
            # Write/save the rotation (overwrite image)
            cv2.imwrite(imgName, img2)
            # Go to next image
            print(angle)
            continue
        elif key & 0xFF == ord('s'): # Skip
            # Go to next image
            continue
run()

"""
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0039.png
64
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0040.png
317
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0041.png
178
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0042.png
127
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0043.png
63
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0044.png
343
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0045.png
268
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0046.png
176
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0047.png
108
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0048.png
102
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0049.png
188
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0050.png
333
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0051.png
92
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0052.png
254
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0053.png
46
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0054.png
220
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0055.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0056.png
121
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0057.png
93
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0058.png
31
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0059.png
328
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0060.png
303
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0061.png
232
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0062.png
167
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0063.png
121
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0064.png
96
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0065.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0066.png
121
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0067.png
150
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0068.png
228
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0069.png
297
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0070.png
37
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0071.png
81
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0072.png
194
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0073.png
323
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0074.png
9
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0075.png
73
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0076.png
210
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0077.png
12
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0078.png
204
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0079.png
15
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0080.png
211
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0081.png
353
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0082.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0083.png
347
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0084.png
98
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0085.png
283
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0086.png
30
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0087.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0088.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0089.png
268
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0090.png
314
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0091.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0092.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0093.png
46
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0094.png
58
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0095.png
14
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0096.png
82
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0097.png
186
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0098.png
336
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0099.png
97
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0100.png
245
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0101.png
62
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0102.png
213
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0103.png
325
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0104.png
58
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0105.png
133
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0106.png
166
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0107.png
213
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0108.png
258
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0109.png
337
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0110.png
51
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0111.png
124
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0112.png
164
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0113.png
188
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0114.png
250
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0115.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0116.png
79
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0117.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0118.png
92
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0119.png
114
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0120.png
177
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0121.png
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0122.png
109
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0123.png
109
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0124.png
123
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0125.png
142
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0126.png
166
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0127.png
186
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0128.png
221
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0129.png
307
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0130.png
33
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0131.png
122
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0132.png
218
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0133.png
339
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0134.png
103
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0135.png
207
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0136.png
312
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0137.png
14
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0138.png
112
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0139.png
251
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0140.png
40
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0141.png
173
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0142.png
288
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0143.png
272
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0144.png
347
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0145.png
85
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0146.png
192
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0147.png
258
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0148.png
316
Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated/thumb0149.png
43
"""
