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

if [ -z "$IN_NIX_SHELL" ]; then
    echo "THis script must be run within a nix-shell"
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
    if [ "$mode" == "sift" ]; then
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
if [ "$mode" == "sift" ]; then
    compileSIFT="make -j4 sift_exe_release_commandLine"
fi
#nix-shell --run "$compileSIFT make -j4 subscale_exe_release" # Problem: this doesn't set the exit code so set -e doesn't exit for it. Instead we use the below and check for nix-shell at the start of this script:
$compileSIFT ; make -j4 subscale_exe_release
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
quiet=1 # set to 0 for verbose
if [ "$mode" == "sift" ]; then
    echo "@@@@ Starting thermocouple temperature recording"
    /usr/bin/python3 WindTunnel/run.py 0 "$quiet" &
else
    echo "@@@@ Starting pi temperature recording"
    #bash -c "while true; do vcgencmd measure_temp ; sleep 0.5; done" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").temperature.log.txt" &
    /usr/bin/python3 WindTunnel/run.py 1 "$quiet" & # 1 = no thermocouple
fi
# Stop SIFT after x seconds:
# if [ "$dontsleep3" != "1" ]; then
#     echo "@@@@ Waiting to stop SIFT"
#     sleep_ 69 # Ensure you don't subtract the above times, since we run the above sleep in the background.
# fi
echo "@@@@ Starting driver for $mode"
if [ "$mode" == "sift" ]; then
    mainDeploymentToTouchDown=74000 # milliseconds
    realFlight="--sift-start-time 20000 --takeoff-g-force 5 --main-deployment-g-force 1 --backup-sift-stop-time $(($mainDeploymentToTouchDown-10000))"
    testing="--sift-start-time 0 --backup-sift-stop-time 20000"
    extraArgs="$testing"
    ./subscale_exe_release --sift-params '-C_edge 2 -delta_min 0.6' --backup-takeoff-time 0 --backup-sift-start-time 10000 $extraArgs 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt"
else
    ./subscale_exe_release --video-capture --sift-start-time "$siftStart" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt"
fi
set +e
cleanup