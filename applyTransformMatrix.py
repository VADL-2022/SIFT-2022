import cv2
import numpy as np

xoff=0
yoff=0
M1 = np.matrix([[2.4273, -4.6812, 1944.4869],
                [2.5873, 1.3817, -1393.4686],
                [0.0020, 0.0017, -0.5943]
               ])
M2 = np.matrix([[-59.3289, 29.2448, 10803.0552],
               [46.2792, -43.2338, -1775.4325],
               [0.0380, -0.0248, -4.0898]])
M3= np.matrix([[-5.977999828068925, -0.9880255704017129, 1419.808234618616],
               [1.324702984623343, -1.788003135189102, -126.9216461371965],
               [-0.009412539975000779, 0.005062239418966106, 1.913349141488792]])
M4=np.matrix([[-114.7408342913017, 16.58993654620327, 12207.12269585698],
               [-68.88147321559404, -17.05208611593818, 16051.28685834884],
              [-0.231843067828002, -0.005023504292161232, 38.05148315615509] # Apply this and above to subscale2 footage first image
              ]) # From /Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-02-17_09_13_39_CST/scaled15...
M5=np.matrix([[9.762088597832031, -0.06470291995465242, -1035.481126733594],
              [2.30978168109985, 6.973031221831425, -1042.933476853166],
              [0.01188577582974148, -0.01071269415985704, 0.7631028038426273]]) # Apply to drone test footage first image
droneTestFirstImage='/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-02-17_17_01_45_CST/firstImage0.png'
M=M5



Mcurrent = np.matrix([[1.0, 0.0, 0.0],
                      [0.0, 1.0, 0.0],
                      [0.0, 0.0, 1.0]])
#inc=0.005*2
inc=0.005/2

# https://medium.com/swlh/youre-using-lerp-wrong-73579052a3c3
def lerp(a, b, t):
    return a + (b-a) * t

imgOrig = cv2.imread(droneTestFirstImage) #cv2.imread("/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-02-17_09_31_47_CST/firstImage0.png")
    #"Data/subscale2/PostFlightSIFTReRuns_derived/output.png")
while True:
    print(imgOrig.shape[:2])
    img = cv2.warpPerspective(imgOrig, Mcurrent, imgOrig.shape[:2])
    cv2.imshow('',img)
    for i in range(0, len(Mcurrent)):
        x = Mcurrent[i]
        for j in range(0, len(x)):
            y = x[j]
            Mcurrent[i][j] = lerp(y, M[i][j], inc)
            #inc*=1.5
    if cv2.waitKey(0) & 0xFF == ord('q'): # https://stackoverflow.com/questions/20539497/python-opencv-waitkey0-does-not-respond
        exit(0)
