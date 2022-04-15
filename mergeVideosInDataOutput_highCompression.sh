#!/usr/bin/env bash

# https://stackoverflow.com/questions/59895/how-can-i-get-the-source-directory-of-a-bash-script-from-within-the-script-itsel
DIR=$(dirname "${BASH_SOURCE[0]}")   # Get the directory name
DIR=$(realpath "${DIR}")    # Resolve its full path if need be

# https://stackoverflow.com/questions/28922352/how-can-i-merge-all-the-videos-in-a-folder-to-make-a-single-video-file-using-ffm/37756628
output="$1/output.mp4"
outputH="$1/output.highCompression.mp4"
#ffmpeg -f concat -i <("$DIR/compareNatSort/compareNatSort" "$1" | sed 's:\ :\\\ :g'| sed 's/^/file /') -c copy "$output"

"$DIR/compareNatSort/compareNatSort" "$1" | sed 's:\ :\\\ :g'| sed 's/^/file /' > "$DIR/fl.txt"
# For some reason this doesn't produce smaller size, but if you do it after the normal merge then it's ok: ffmpeg -f concat -safe 0 -i "$DIR/fl.txt" -c:v libx265 -vtag hvc1 -c:a copy "$output"

# Note: https://superuser.com/questions/922866/ffmpeg-skip-process-if-output-already-exists : {"
# -n (global)
# Do not overwrite output files, and exit immediately if a specified output file already exists.
# "}
ffmpeg -n -f concat -safe 0 -i "$DIR/fl.txt" -c copy "$output"

# Get video duration in seconds
# https://superuser.com/questions/650291/how-to-get-video-duration-in-seconds
# seconds=$(ffmpeg -i "$input" 2>&1 | grep "Duration"| cut -d ' ' -f 4 | sed s/,// | sed 's@\..*@@g' | awk '{ split($1, A, ":"); split(A[3], B, "."); print 3600*A[1] + 60*A[2] + B[1] }')
# # Compress and cut video in half (in half because it starts at apogee on SIFT1 but not sift2??)
# # if [ "$(hostname)" == "sift2" ]; then
# #     seconds=0
# # fi
# if [ "$seconds" -lt "20" ]; then
#    seconds=0
# fi
# ffmpeg -ss 00:00:$(($seconds / 2)) -i "$input" -movflags faststart -crf 40 -vf "scale=320:-1, format=gray" live69_out.mp4

ffmpeg -n -i "$output" -c:v libx265 -vtag hvc1  -b 200k -c:a copy "$outputH"

rm "$DIR/fl.txt"
echo "$outputH"
