pushd .
cd ..
make -j4 sift_exe_release_commandLine
popd

#SHELL=/usr/bin/env bash
OS=$(uname -s)
if [ "$OS" == "Darwin" ]; then
    # https://stackoverflow.com/questions/16387484/clangllvm-compile-with-frameworks
    CC="xcrun -sdk macosx clang"
    CXX="xcrun -sdk macosx clang++"
    CFLAGS="$CFLAGS -target x86_64-apple-macos10.15"
    LFLAGS="$LDFLAGS -lc++fs"
    LFLAGS="$LFLAGS $LDFLAGS -framework CoreGraphics -framework Foundation"
else
    CC="clang"
    CXX="clang++"
    LFLAGS="$LDFLAGS -lX11"
fi

$CXX -I../src -g3 -O0 $CFLAGS $LFLAGS `pkg-config --libs opencv4 libpng` ../src/Timer_release_commandLine.o ../src/common_release_commandLine.o ../src/utils_release_commandLine.o -DSIFT_IMPL=SIFTAnatomy -DSIFTAnatomy_ -std=c++17 -I../sift_anatomy_20141201/src ../sift_anatomy_20141201/src/io_png_release_commandLine.o ../src/strnatcmp_release_commandLine.o ../sift_anatomy_20141201/src/lib_util_release_commandLine.o -I/nix/store/9gk5f9hwib0xrqyh17sgwfw3z1vk9ach-opencv-4.5.2/include/opencv4 -I/nix/store/gzmldcqhy7scwriamp4jlvqzhm6s2kqg-opencv-4.5.2/include/opencv4/ $NIX_CFLAGS_COMPILE $NIX_LDFLAGS ../src/DataSource_release_commandLine.o ../commonOutMutex_release_commandLine.o ../src/pthread_mutex_lock__release_commandLine.o ../src/main/siftMainCmdConfig_release_commandLine.o ../src/tools/backtrace/backtrace_release_commandLine.o ../src/tools/backtrace/demangle_release_commandLine.o "$@"
