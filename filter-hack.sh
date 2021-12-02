# Remove the linker flag that starts with `-L` and ends with `-opencv-4.5.2/lib`
echo "$1" | awk 'BEGIN{RS=" "; ORS=" ";} !/\-L.*-opencv\-4.5.2\/lib/ {print}'
