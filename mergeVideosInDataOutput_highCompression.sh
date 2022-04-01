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
ffmpeg -f concat -safe 0 -i "$DIR/fl.txt" -c copy "$output"
ffmpeg -i "$output" -c:v libx265 -vtag hvc1  -b 200k -c:a copy "$outputH"
rm "$DIR/fl.txt"
echo "$outputH"
