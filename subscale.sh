# https://stackoverflow.com/questions/35800082/how-to-trap-err-when-using-set-e-in-bash
set -eE  # same as: `set -o errexit -o errtrace`
trap 'echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ Failed to prepare subscale @@@@@@@@@@@@@@@@@@@@@@@@@@"' ERR 

git checkout submodule
make -j4 sift_exe_release_commandLine
# sha512 Checksum
./sift_exe_release_commandLine --main-mission --no-preview-window --sift-params -C_edge 2 &
cd ../VADL2021-Source
# sha512 Checksum
./exe/MAIN or DATA -- test it, which is it?
