# https://stackoverflow.com/questions/35800082/how-to-trap-err-when-using-set-e-in-bash
set -eE  # same as: `set -o errexit -o errtrace`
trap 'echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ Failed to prepare subscale @@@@@@@@@@@@@@@@@@@@@@@@@@"' ERR 

sleep_ () {
    echo "Sleeping for $@"
    sleep "$@"
}

git checkout subscale
# TODO: checkout submodules
make -j4 sift_exe_release_commandLine
# sha512 Checksum
make -j4 subscale_exe_release
echo "@@@@ Sleeping before takeoff"
# Sleep x seconds:
sleep_ 30
echo "@@@@ Starting driver"
# SIFT start time in milliseconds:
sudo ./subscale_exe_release --sift-start-time 26000 --sift-only 2>&1 | sudo tee "./dataOutput/$(date +"%Y_%m_%d_%I_%M_%p").log.txt" &
# Stop SIFT after x seconds:
echo "@@@@ Waiting to stop SIFT"
sleep_ $((69-26)) # Ensure to subtract times, except first sleep, from the above in seconds
echo "@@@@ Stopping SIFT"
pkill -SIGINT sift
# sha512 Checksum
