file="/Volumes/Seagate/SeniorSemester1_Vanderbilt_University/RocketTeam/GDriveRClone/VADL '21-22/VADL '20-21/VADL '19-20/VADL '18-19/VADL 2017-2018/Videos/Flight Filtering/Flight 3 Video .mp4"

mkdir -p outFrames
ffmpeg -i "$file" -r 1/1 -vsync 0 outFrames/out%d.png
