# Start with `bash ./subscale.sh`

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

if [ "$(pwd)" != "/home/pi/VanderbiltRocketTeam" ]]; then
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
# TODO: checkout submodules
sudo -H -u pi /home/pi/.nix-profile/bin/nix-shell --run "make -j4 sift_exe_release_commandLine"
# sha512 Checksum
sudo -H -u pi /home/pi/.nix-profile/bin/nix-shell --run "make -j4 subscale_exe_release"
if [ "$dontsleep" == "1" ]; then
    echo "@@@@ Sleeping before takeoff"
    
    # Sleep x seconds:
    sleep_ 30
fi
echo "@@@@ Starting driver"
# SIFT start time in milliseconds:
sudo ./subscale_exe_release --sift-params '-C_edge 2' --sift-start-time 26000 --sift-only 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").log.txt" &
# Record temperature data
bash -c "while true; do vcgencmd measure_temp ; sleep 0.5; done" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").temperature.log.txt" &
# Stop SIFT after x seconds:
if [ "$dontsleep" == "1" ]; then
    echo "@@@@ Waiting to stop SIFT"
    sleep_ 69 # Ensure you don't subtract the above times, since we run the above sleep in the background.
fi
echo "@@@@ Stopping SIFT"
pkill -SIGINT sift
# Stop temperature data
pkill -SIGINT vcgencmd
# sha512 Checksum
