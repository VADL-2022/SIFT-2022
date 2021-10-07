actual_path=$(readlink -f "${BASH_SOURCE[0]}")
script_dir=$(dirname "$actual_path")
/usr/bin/rsync --backup --archive --verbose --human-readable --progress "$script_dir"/ pi@raspberrypi.local:/home/pi/VanderbiltRocketTeam/ --exclude='*.o' --exclude='quadcopter' --exclude='example' --exclude='camera' --exclude='.git'
