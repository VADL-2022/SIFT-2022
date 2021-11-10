cd sift_anatomy_20141201/ && make clean && make -j8 && cd .. && make clean && make -j8 sift_exe && ./sift_exe 0
echo 'ON RPI: export DISPLAY=:0.0 && ./sift_exe' # https://github.com/opencv/opencv/issues/18461
