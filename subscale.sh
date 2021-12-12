# https://stackoverflow.com/questions/35800082/how-to-trap-err-when-using-set-e-in-bash
set -eE  # same as: `set -o errexit -o errtrace`
trap 'echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ Failed to prepare subscale @@@@@@@@@@@@@@@@@@@@@@@@@@"' ERR 

git checkout subscale
# TODO: checkout submodules
make -j4 sift_exe_release_commandLine
# sha512 Checksum
make -j4 subscale_exe_release
sudo ./subscale_exe_release --sift-start-time 0 --sift-only
# sha512 Checksum
