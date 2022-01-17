#!/usr/bin/env bash

# https://stackoverflow.com/questions/59895/how-can-i-get-the-source-directory-of-a-bash-script-from-within-the-script-itsel
DIR=$(dirname "${BASH_SOURCE[0]}")   # Get the directory name
DIR=$(realpath "${DIR}")    # Resolve its full path if need be

# https://stackoverflow.com/questions/28922352/how-can-i-merge-all-the-videos-in-a-folder-to-make-a-single-video-file-using-ffm/37756628
output="$(dirname "$1")/output.mp4"
#ffmpeg -f concat -i <("$DIR/compareNatSort/compareNatSort" "$1" | sed 's:\ :\\\ :g'| sed 's/^/file /') -c copy "$output"

"$DIR/compareNatSort/compareNatSort" "$1" | sed 's:\ :\\\ :g'| sed 's/^/file /' > "$DIR/fl.txt"
ffmpeg -f concat -i "$DIR/fl.txt" -c copy "$output"
rm "$DIR/fl.txt"
