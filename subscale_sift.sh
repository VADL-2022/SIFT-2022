# Start with `bash ./subscale_sift.sh`

# if [[ $EUID -ne 0 ]]; then
#    echo "This script must be run as root" 
#    exit 1
# fi
if [[ $(whoami) -ne pi ]]; then
   echo "This script must be run as the pi user" 
   exit 1
fi

if [ "$(pwd)" != "/home/pi/VanderbiltRocketTeam" ]; then
   echo "This script must be run from /home/pi/VanderbiltRocketTeam" 
   exit 1
fi

dontsleep="$1"
dontsleep2="$2"
dontsleep3="$3"

cleanup() {
    echo "@@@@ Stopping SIFT"
    pkill -SIGINT sift
    # Stop temperature data
    #pkill -f vcgencmd
    pkill -f 'python3 WindTunnel'
    # sha512 Checksum

    exit
}

ctrl_c() {
    set +o errexit +o errtrace
    echo "** Trapped CTRL-C"
    cleanup
}

onErr() {
    set +o errexit +o errtrace
    echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ Failed to prepare subscale @@@@@@@@@@@@@@@@@@@@@@@@@@"
    cleanup
}

# https://stackoverflow.com/questions/35800082/how-to-trap-err-when-using-set-e-in-bash
set -eE  # same as: `set -o errexit -o errtrace`
trap onErr ERR 

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

sleep_ () {
    echo "Sleeping for $@ seconds"
    sleep "$@"
}

git checkout subscale
# Checkout submodules from the above commit as well:
git submodule update --init --recursive 
/home/pi/.nix-profile/bin/nix-shell --run "make -j4 sift_exe_release_commandLine; make -j4 subscale_exe_release"
# TODO: sha512 Checksum
if [ "$dontsleep" != "1" ]; then
    echo "@@@@ Sleeping before takeoff"
    
    # Sleep x seconds:
    sleep_ 30
fi
echo "@@@@ Starting driver"
# SIFT start time in milliseconds:
if [ "$dontsleep2" != "1" ]; then
    siftStart=10000
else
    siftStart=0
fi
# sudo ./subscale_exe_release --sift-params '-C_edge 2' --sift-start-time "$siftStart" --sift-only 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").log.txt" &
./subscale_exe_release --sift-params '-C_edge 2 -delta_min 0.6' --sift-start-time "$siftStart" 
# Record temperature data
set +o errexit +o errtrace
#bash -c "while true; do vcgencmd measure_temp ; sleep 0.5; done" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").temperature.log.txt" &
python3 WindTunnel/run.py &
# Stop SIFT after x seconds:
if [ "$dontsleep3" != "1" ]; then
    echo "@@@@ Waiting to stop SIFT"
    sleep_ 69 # Ensure you don't subtract the above times, since we run the above sleep in the background.
fi
cleanup
