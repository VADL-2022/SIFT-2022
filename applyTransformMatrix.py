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

M6=np.matrix([[460.1593113915993, -5.925398318608927, -143295.5350288748],
              [-96.68889393599359, -71.37197210965851, 43526.01193106594],
              [0, 0, 0.9999999999999983]]) # On git commit 7d743f75c4a2924a7bd3d3bea7184c6ddefb04ba: ./sift_exe_release_commandLine --sift-params -delta_min 0.6 -C_edge 2 --video-file-data-source --video-file-data-source-path /Volumes/MyTestVolume/Projects/DataRocket/files_sift1_videosTrimmedOnly_fullscale1/2022-02-19_11_31_58_CST/output.mp4  --main-mission --skip-image-indices 2 2 --skip-image-indices 5 5 --skip-image-indices 7 7 --skip-image-indices 14 14 --skip-image-indices 17 19  --skip-image-indices 21 24 --skip-image-indices 32 38 --skip-image-indices 55 64  --skip-image-indices 65 88 --skip-image-indices 89 91 --skip-image-indices 94 94 --skip-image-indices 100 108 --skip-image-indices 110 110 --skip-image-indices 111 121 --skip-image-indices 122 129 --skip-image-indices 141 141
# Testing:
"""
./sift_exe_release_commandLine --sift-params -delta_min 0.6 -C_edge 2 --video-file-data-source --video-file-data-source-path /Volumes/MyTestVolume/Projects/DataRocket/files_sift1_videosTrimmedOnly_fullscale1/2022-02-19_11_31_58_CST/output.mp4  --main-mission --skip-image-indices 2 2 --skip-image-indices 5 5 --skip-image-indices 7 7 --skip-image-indices 14 14 --skip-image-indices 17 19  --skip-image-indices 21 24 --skip-image-indices 32 38 --skip-image-indices 55 64  --skip-image-indices 65 88 --skip-image-indices 89 108 --skip-image-indices 110 110 --skip-image-indices 111 141
""" # Gave:
M7=np.matrix([[-243.3613235845537, 167.141896995625, 38588.78930223091],
              [ 151.2045618626928, -109.7610614449212, -18512.55206900018],
              [ 0, 0, 0.9999999999999987]])
fullscale1FirstImage='/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-02-28_20_22_58_CST/firstImage0.png'
M8=np.matrix([[-4.123747055240452, 6.85419246591674, -886.9633044039938],
              [ -13.2435252777686, 5.228203715333559, 1375.028279449747],
               [0, 0, 0.9999999999999988]])
M9=np.matrix([[41.1917516380786, -7.77848942663927, -3287.620132284611],
               [-48.01547406853982, 16.46625658697471, 1380.5892588642],
              [0, 0, 0.9999999999999991]])
M10=np.matrix([[2.670736086272905, 0.7030058102099318, -924.8139413316903],
               [-0.8835416184730005, 1.922214914131477, -82.96614850316857],
               [0, 0, 0.9999999999999993]]) # "The good matrix"
M11=np.matrix([[10.79171218627111, 0.7321998571250317, -2021.863408093771],
 [              -12.61648918272472, 0.9959594705167039, 1845.892362057198],
[ 0, 0, 0.9999999999999992]])
M12=np.matrix([[41.1917516380786, -7.77848942663927, -3287.620132284611],
               [-48.01547406853982, 16.46625658697471, 1380.5892588642],
               [ 0, 0, 0.9999999999999991]])
M13=np.matrix([[8.547295697767519, 0.2485540338071431, -1309.752223368877],
               [-9.955630524461194, 1.349300720856145, 800.017392094951],
               [ 0, 0, 0.9999999999999993]])
M14=np.matrix([[41.61292051847861, -20.33492628578671, 6872.431092275944],
               [-57.92623005240372, 46.95245080138437, -17192.79901791887],
               [0, 0, 0.9999999999999992]]) # Command: lldb -o run -- ./sift_exe_debug_commandLine --video-file-data-source --video-file-data-source-path /Volumes/MyTestVolume/Projects/DataRocket/files_sift1_videosTrimmedOnly_fullscale1/2022-02-19_11_31_58_CST/output.mp4 --main-mission --sift-video-output --finish-rest-always --skip-image-indices 2 3 --skip-image-indices 4 6 --skip-image-indices 7 8 --skip-image-indices 14 15 --skip-image-indices 18 21 --skip-image-indices 22 24 --skip-image-indices 25 29 --skip-image-indices 32 33 --skip-image-indices 35 37 --skip-image-indices 39 41 --skip-image-indices 42 43 --skip-image-indices 44 46 --skip-image-indices 47 50 --skip-image-indices 51 55 --skip-image-indices 56 57 --skip-image-indices 58 59 --skip-image-indices 60 61 --skip-image-indices 64 66 --skip-image-indices 67 70 --skip-image-indices 71 74 --skip-image-indices 75 80 --skip-image-indices 81 82 --skip-image-indices 83 85 --skip-image-indices 86 88 --skip-image-indices 89 92 --skip-image-indices 94 95 --skip-image-indices 96 97 --skip-image-indices 99 111 --skip-image-indices 112 117 --skip-image-indices 118 122 --skip-image-indices 123 125 --skip-image-indices 127 130 --skip-image-indices 132 136 --skip-image-indices 139 140 --skip-image-indices 141 142 #--skip-image-indices 146 14 --debug-mutex-deadlocks
M15=np.matrix([[39.94850471770674, -116.5479015650005, 28957.58515670232],
                [25.80858786100335, -208.3688259967057, 54460.57806560803],
               [ 0, 0, 0.9999999999999991]])
M16=np.matrix([[-10.79361828440029, 30.02665863387952, -5796.683949147701],
               [-14.24970743128583, -24.28169240383916, 6492.762115021626],
               [0, 0, 0.9999999999999994]])
M17=np.matrix([[4.948867896701391, -0.7664984793150436, -505.8483297718342],
               [ -1.455364274725157, 3.438942450270565, -41.62095821273624],
               [ 0, 0, 0.9999999999999996]]) # /Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-03-20_00_37_21_CDT/scaled22.png.matrix0.txt
#M=M10
M=M17
#imgPath=fullscale1FirstImage
imgPath='/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-03-20_00_19_34_CDT/firstImage0.png'


idMat=np.matrix([[1.0, 0.0, 0.0],
                 [0.0, 1.0, 0.0],
                 [0.0, 0.0, 1.0]])
Mcurrent = idMat.copy()
#inc=0.005*2
inc=0.005/2
incOrig=inc

# https://medium.com/swlh/youre-using-lerp-wrong-73579052a3c3
def lerp(a, b, t):
    return a + (b-a) * t

#imgOrig = cv2.imread(droneTestFirstImage) #cv2.imread("/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-02-17_09_31_47_CST/firstImage0.png")
    #"Data/subscale2/PostFlightSIFTReRuns_derived/output.png")
imgOrig = cv2.imread(imgPath)
while True:
    print(imgOrig.shape[:2])
    widthAndHeight=np.array(imgOrig.shape[:2])
    temp=widthAndHeight[0]
    widthAndHeight[0] = widthAndHeight[1]
    widthAndHeight[1]=temp
    print(widthAndHeight)
    img = cv2.warpPerspective(imgOrig, Mcurrent, widthAndHeight)
    cv2.imshow('',img)
    cv2.resizeWindow('', widthAndHeight[0], widthAndHeight[1])
    for i in range(0, len(Mcurrent)):
        x = Mcurrent[i]
        for j in range(0, len(x)):
            y = x[j]
            Mcurrent[i][j] = lerp(y, M[i][j], inc)
            #inc*=1.5
    key = cv2.waitKey(0)
    if key & 0xFF == ord('q'): # https://stackoverflow.com/questions/20539497/python-opencv-waitkey0-does-not-respond
        exit(0)
    while True:
        if key & 0xFF == ord('f'): # Faster
            inc+=inc*0.1
        elif key & 0xFF == ord('g'): # Faster
            inc+=0.001
        elif key & 0xFF == ord('s'): # Slower
            inc-=0.001
        elif key & 0xFF == ord('x'): # Slower
            inc-=inc*0.1
        elif key & 0xFF == ord('i'): # Identity
            Mcurrent=idMat.copy()
            inc = incOrig
            img=imgOrig.copy()
            break
        elif key & 0xFF == ord('m'): # Use matrix
            Mcurrent=M.copy()
            break
        else:
            break
        print(inc)
        key = cv2.waitKey(0)
    print(Mcurrent)
