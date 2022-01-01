# Start with `bash ./subscale.sh`

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

if [ "$(pwd)" != "/home/pi/VanderbiltRocketTeam" ]; then
   echo "This script must be run from /home/pi/VanderbiltRocketTeam" 
   exit 1
fi

dontsleep="$1"

# https://stackoverflow.com/questions/35800082/how-to-trap-err-when-using-set-e-in-bash
set -eE  # same as: `set -o errexit -o errtrace`
trap 'echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ Failed to prepare subscale @@@@@@@@@@@@@@@@@@@@@@@@@@"' ERR 

sleep_ () {
    echo "Sleeping for $@ seconds"
    sleep "$@"
}

git checkout subscale
# sha512 Checksum
sudo -H -u pi /home/pi/.nix-profile/bin/nix-shell --run "make -j4 subscale_exe_release"
if [ "$dontsleep" != "1" ]; then
    echo "@@@@ Sleeping before takeoff"
    
    # Sleep x seconds:
    sleep_ 30
fi
echo "@@@@ Starting driver"
# SIFT start time in milliseconds:
sudo ./subscale_exe_release --sift-start-time 26000 --imu-record-only 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").video_cap.log.txt" &
# Stop SIFT after x seconds:
/home/pi/.nix-profile/bin/nix-shell --run "echo sudo -H -u pi /home/pi/.nix-profile/bin/nix-shell --run \"/nix/store/ga036m4z5f5g459f334ma90sp83rk7wv-python3-3.9.6-env/bin/python3 ./subscale_driver/videoCapture.py\""
#if [ "$dontsleep" != "1" ]; then
    echo "@@@@ Waiting to stop video cap"
    sleep_ 69 # Ensure you don't subtract the above times, since we run the above sleep in the background.
#fi
echo "@@@@ Stopping video cap"
set +e
pkill -SIGINT subscale
pkill -SIGINT videoCapture # Kill python video capture
# sha512 Checksum