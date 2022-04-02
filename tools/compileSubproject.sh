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
    CFLAGS="$CFLAGS -target x86_64-apple-macos10.15 -I/nix/store/kqqwh1xz3ri47dvg8ikj7w2yl344amw3-python3-3.7.11/include -I/nix/store/690a3qx1w73vd86aawgih3fv5bn393lf-python3.7-pybind11-2.7.0/include"
    LFLAGS="$LDFLAGS -lc++fs"
    LFLAGS="$LFLAGS $LDFLAGS -framework CoreGraphics -framework Foundation"
else
    CC="clang"
    CXX="clang++"
    LFLAGS="$LDFLAGS -lX11"
fi

# Hack to fix
# /nix/store/7whwzj6l52qcnlg8ym82gmzsdr9x5p20-binutils-2.35.1/bin/ld: ../src/tools/backtrace/backtrace_release_commandLine.o: undefined reference to symbol '_ULaarch64_get_proc_name'
# /nix/store/7whwzj6l52qcnlg8ym82gmzsdr9x5p20-binutils-2.35.1/bin/ld: /nix/store/xfbzmmgkk4frznwfkl72d6lrhgw092qw-libunwind-1.4.0/lib/libunwind.so.8: error adding symbols: DSO missin
# g from command line
# https://stackoverflow.com/questions/19901934/libpthread-so-0-error-adding-symbols-dso-missing-from-command-line :
hack="-Wl,--copy-dt-needed-entries"

$CXX $hack -I../src -I../VectorNav/include -g3 -O0 $CFLAGS $LFLAGS `pkg-config --cflags --libs opencv4 libpng python-3.7m` ../src/Timer_release_commandLine.o ../src/common_release_commandLine.o ../src/utils_release_commandLine.o -DSIFT_IMPL=SIFTAnatomy -DSIFTAnatomy_ -std=c++17 -I../sift_anatomy_20141201/src ../sift_anatomy_20141201/src/io_png_release_commandLine.o ../src/strnatcmp_release_commandLine.o ../sift_anatomy_20141201/src/lib_util_release_commandLine.o -I/nix/store/9gk5f9hwib0xrqyh17sgwfw3z1vk9ach-opencv-4.5.2/include/opencv4 -I/nix/store/gzmldcqhy7scwriamp4jlvqzhm6s2kqg-opencv-4.5.2/include/opencv4/ $NIX_CFLAGS_COMPILE $NIX_LDFLAGS ../src/DataSource_release_commandLine.o ../commonOutMutex_release_commandLine.o ../src/pthread_mutex_lock__release_commandLine.o ../src/main/siftMainCmdConfig_release_commandLine.o ../src/tools/backtrace/backtrace_release_commandLine.o ../src/tools/backtrace/demangle_release_commandLine.o "$@"
