import os
from moviepy.editor import *

clips = []
inputDir = '/home/pi/VanderbiltRocketTeam/Summer2022/dataOutput/2022-08-15_09_58_41_'

print('Searching for files')
for filename in os.listdir(inputDir): # make array of files
    clips.append(inputDir + '/' + filename)
        
        for
clips.sort() # put files in correct order


# make into videoFileClip objects
for filename in clips
    clips.
print('Files found. Concatenating now...')

video = concatenate_videoclips(clips) # combine
video.write_videofile('merged.mp4') # write video
print('Video compiled successfully')