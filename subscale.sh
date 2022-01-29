#!/usr/bin/env bash
# Start with `bash ./subscale.sh`

if [[ "$(whoami)" != "pi" ]]; then
   echo "This script must be run as \"pi\""
   exit 1
fi

if [ "$(pwd)" != "/home/pi/VanderbiltRocketTeam" ]; then
   echo "This script must be run from /home/pi/VanderbiltRocketTeam" 
   exit 1
fi

hostname=$(hostname)
if [[ "$hostname" =~ ^sift.* ]]; then
    mode=sift
    echo "This is a SIFT pi (hostname starts with \"sift\": $hostname)"
else
    mode=video_cap
    echo "This is a video capture pi (hostname doesn't start with \"sift\": $hostname)"
fi

dontsleep="$1"
dontsleep2="$2"
dontsleep3="$3"

cleanup() {
    echo "@@@@ Stopping programs"
    if [ "$videoCapture" == "sift" ]; then
	pkill -SIGINT sift
    fi
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

git checkout subscale2
# Checkout submodules from the above commit as well:
git submodule update --init --recursive
if [ "$videoCapture" == "sift" ]; then
    compileSIFT="make -j4 sift_exe_release_commandLine;"
fi
nix-shell --run "$compileSIFT make -j4 subscale_exe_release"
# TODO: sha512 Checksum
# if [ "$dontsleep" != "1" ]; then
#     echo "@@@@ Sleeping before takeoff"
    
#     # Sleep x seconds:
#     sleep_ 30
# fi

# # SIFT start time in milliseconds:
# if [ "$dontsleep2" != "1" ]; then
#     siftStart=26000
# else
#     siftStart=0
# fi
# Record temperature data
#set +o errexit +o errtrace
if [ "$videoCapture" == "sift" ]; then
    echo "@@@@ Starting thermocouple temperature recording"
    /usr/bin/python3 WindTunnel/run.py &
else
    echo "@@@@ Starting pi temperature recording"
    #bash -c "while true; do vcgencmd measure_temp ; sleep 0.5; done" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").temperature.log.txt" &
    /usr/bin/python3 WindTunnel/run.py 1 & # 1 = no thermocouple
fi
# Stop SIFT after x seconds:
# if [ "$dontsleep3" != "1" ]; then
#     echo "@@@@ Waiting to stop SIFT"
#     sleep_ 69 # Ensure you don't subtract the above times, since we run the above sleep in the background.
# fi
echo "@@@@ Starting driver for $mode"
if [ "$videoCapture" == "sift" ]; then
    ./subscale_exe_release --sift-params '-C_edge 2 -delta_min 0.6' --sift-start-time "$siftStart" --backup-takeoff-time 0 --backup-sift-stop-time 4000 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt"
else
    ./subscale_exe_release --video-capture --sift-start-time "$siftStart" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt"
fi
set +e
cleanup
