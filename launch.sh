#!/usr/bin/env bash
# Start with `bash ./launch.sh`

if [[ "$(whoami)" != "pi" ]]; then
   echo "This script must be run as \"pi\""
   exit 1
fi

# Sync network time
timedatectl

# Login script for logging your ip in case connection lost
if [ ! -e ~/.loginScriptV1Added ]; then
    echo -e "\n\n#loginScriptV1Added#\nifconfig\n#End loginScriptV1Added#\n" >> ~/.bashrc
    touch ~/.loginScriptV1Added
fi

if [ "$(pwd)" != "/home/pi/VanderbiltRocketTeam" ]; then
   echo "This script must be run from /home/pi/VanderbiltRocketTeam" 
   exit 1
fi

if [ -z "$IN_NIX_SHELL" ]; then
    echo "THis script must be run within a nix-shell"
    exit 1
fi

# Disable ASLR for slight speedup
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space # https://askubuntu.com/questions/318315/how-can-i-temporarily-disable-aslr-address-space-layout-randomization

hostname=$(hostname)
if [[ "$hostname" =~ ^sift.* ]]; then
    mode=sift
    echo "This is a SIFT pi (hostname starts with \"sift\": $hostname)"
else
    mode=video_cap
    echo "This is a video capture pi (hostname doesn't start with \"sift\": $hostname)"
fi

#testingExtras="$2"
#dontsleep="$2"
#dontsleep2="$3"
#dontsleep3="$4"

cleanup() {
    echo "@@@@ Stopping programs"
    if [ "$mode" == "sift" ]; then
	pkill -SIGINT sift
    fi
    # Stop temperature data
    #pkill -f vcgencmd
    #pkill -SIGINT -f 'import WindTunnel.run'
    pkill -SIGTERM -f 'import WindTunnel.run' # Python matches this
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

# Trap on exit script
trap cleanup EXIT

sleep_ () {
    echo "Sleeping for $@ seconds"
    sleep "$@"
}

gitBranch="nasa"
# if [ -z "$gitBranch" ]; then
#     gitBranch="fullscale3"
# fi
git checkout "$gitBranch"
# Checkout submodules from the above commit as well:
git submodule update --init --recursive
if [ "$mode" == "sift" ]; then
    compileSIFT="make -j4 sift_exe_release_commandLine"
fi
#nix-shell --run "$compileSIFT make -j4 subscale_exe_release" # Problem: this doesn't set the exit code so set -e doesn't exit for it. Instead we use the below and check for nix-shell at the start of this script:
#exe=subscale_exe_release
exe=subscale_exe_release
#$compileSIFT
make -j4 $exe

cd compareNatSort
bash compile.sh
cd ..

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
# For WindTunnel #
#sudo pip3 install RPi.GPIO
#sudo pip3 install adafruit-circuitpython-max31855
#sudo pip3 install adafruit-blinka
# #
windTunnel="import sys; sys.path = list(filter(lambda x: '/nix/store/' not in x, sys.path)); import WindTunnel.run #That's all folks"
fps=
if [ "$mode" == "sift" -a "$hostname" == "sift1" ]; then
    fps=5
fi
if [ "$mode" == "sift" -a "$hostname" == "sift2" ]; then # -a is "and"..
    echo "@@@@ Starting thermocouple temperature recording"
    /usr/bin/python3 -c "$windTunnel" 0 "$quiet" &

    # for one sift pi: hostname: sift2: check for drogue or apogee instead and start at drogue
    siftPayloadArgs="--start-sift-at-apogee"
    fps=15
else
    echo "@@@@ Starting pi temperature recording"
    #bash -c "while true; do vcgencmd measure_temp ; sleep 0.5; done" 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").temperature.log.txt" &
    /usr/bin/python3 -c "$windTunnel" 1 "$quiet" & # 1 = no thermocouple
fi

# Stop SIFT after x seconds:
# if [ "$dontsleep3" != "1" ]; then
#     echo "@@@@ Waiting to stop SIFT"
#     sleep_ 69 # Ensure you don't subtract the above times, since we run the above sleep in the background.
# fi

echo "@@@@ Starting driver for $mode"
if [ "$mode" == "sift" ]; then
    takeoffGForce=6
else
    takeoffGForce=5
fi
landingGForce=3
backupTakeoffTime=0
timeToMeco=1700
timeToApogee=9500
timeToMainDeployment=17000 # Originally we were going to start SIFT on main deployment. But we use this instead since unsure about IMU trigger on 1 g of main deployment, and there's a fallback via --time-to-main-deployment for this.
mainDeploymentAltitude=800
mainStabilizationTime=1000 # Time to allow rocket to stabilize after main deployment, then running SIFT.
mainDescentTime=33500 
launchBox=$1
launchAngle=$2
siftAllowanceForStopping=0 # Time to take away from mainDescentTime for stopping SIFT in backupSIFTStopTime. Larger values stop SIFT sooner.
backupSIFTStopTime="$(($mainDescentTime-$siftAllowanceForStopping))" # Originally we were going to stop SIFT on altitude data. But we're unsure about altitude data being reliable, so we don't use it to stop SIFT, and there's a fallback via backupSIFTStopTime.

forePayloadArgs="--video-capture --LIS331HH-imu-calibration-file "driver/LIS331HH_calibration/LOG_20220129-183224.csv""

siftPayloadArgs="$siftPayloadArgs --time-to-main-deployment $timeToMainDeployment --time-for-main-stabilization $mainStabilizationTime --main-descent-time $backupSIFTStopTime --emergency-main-deployment-g-force 20 --main-deployment-altitude $mainDeploymentAltitude --launch-box $launchBox --launch-angle $launchAngle"

realFlight="--takeoff-g-force $takeoffGForce --landing-g-force $landingGForce --backup-takeoff-time $backupTakeoffTime --time-to-meco $timeToMeco --time-to-apogee $timeToApogee"

# Add overrides to realFlight for testing:
testing="$realFlight --takeoff-g-force 1 --landing-g-force 1"
if [ "$mode" == "video_cap" ]; then
    testing="$testing --time-to-apogee 5000 --fore-stop-time 5000"
fi

#extraArgs="$testing"
extraArgs="$realFlight"


if [ "$mode" == "sift" ]; then
    ./$exe $siftPayloadArgs --extra-sift-exe-args "--no-sky-detection --fps $fps --no-preview-window --finish-rest-always --save-first-image" --sift-params '-C_edge 2 -delta_min 0.6' $extraArgs 2>&1 | tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt" #| tee <(python3 "driver/radio.py" 1)
else
    #./subscale_exe_release --video-capture --time-for-main-stabilization "$siftStart" 2>&1 | tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt"
    ./$exe $forePayloadArgs $extraArgs 2>&1 | tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%S_%p").$mode""log.txt"
fi
set +e
#cleanup
