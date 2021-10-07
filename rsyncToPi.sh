/usr/bin/rsync --backup --archive --verbose --human-readable --progress $(realpath .) pi@raspberrypi.local:/home/pi/ --exclude='*.o' --exclude='quadcopter' --exclude='example' --exclude='camera'
