# Type a script or drag a script file from your workspace to insert its path.
if [ -z "$IN_NIX_SHELL" ]; then
    if [ -e ~/.nix-profile/etc/profile.d/nix.sh ]; then
        source ~/.nix-profile/etc/profile.d/nix.sh
    fi # added by Nix installer
    
    #~/.nix-profile/bin/nix-shell --run "bash \"$0\"" -p opencv pkgconfig
    cd "$SRCROOT/.."
    ~/.nix-profile/bin/nix-shell --run "bash \"$0\""
    exit 0
fi

SIFT="sift_anatomy_20141201"
SIFT_SRC="$SIFT/src"
XCCONFIG="Config.xcconfig"
#truncate –s 0 "$XCCONFIG"
echo `which pkg-config`
echo "$OPENCV_CFLAGS"
IFLAGS="$(pkg-config --cflags-only-I opencv4 libpng jemalloc gl glew IL) -I"'$(SRCROOT)/../'"$SIFT_SRC -I/nix/store/zflx47lr00hipvkl5nncd2rnpzssnni6-backward-1.6/include -I/nix/store/i0rmp3app7yqd37ihgxlx9c3lwsj16kq-opencl-headers-2020.06.16/include"
echo "$IFLAGS"

# Use a heredoc:
cat <<InputComesFromHERE > "$XCCONFIG"
CXXFLAGS = -std=c++17 \$(CXXFLAGS)
OPENCV_CFLAGS=`pkg-config --cflags opencv4 libpng`
HEADER_SEARCH_PATHS=$(echo -n "$IFLAGS" | awk 'BEGIN { ORS=" "; RS = " " } ; { sub(/^-I/, ""); print $0 }')

OTHER_LDFLAGS = -lm -lpthread `pkg-config --libs opencv4 libpng jemalloc gl glew glut IL` -framework OpenCL -L/nix/store/g5w2hcjkax12ixx8wq1g6r6nqri15ggx-libunwind-7.1.0/lib // Works but segfaults: -Wl,-alias,_malloc,vadlTemp -Wl,-alias,___wrap_malloc,_malloc -Wl,-alias,vadlTemp,___real_malloc -Wl,-alias,_free,vadlTemp2 -Wl,-alias,___wrap_free,_free -Wl,-alias,vadlTemp2,___real_free //-Wl,-alias,_malloc,___wrap_malloc -Wl,-alias,_free,___wrap_free -Wl,-alias,___real_malloc,_malloc -Wl,-alias,___real_free,_free //-Wl,--wrap,malloc -Wl,--wrap,free //-ljpeg -lrt -lm

CLANG_CXX_LIBRARY=libc++
CLANG_CXX_LANGUAGE_STANDARD=c++17

//impl = SIFT_IMPL=SIFTGPU SIFTGPU_ // <-- Be sure to change both of these defines for an impl change
impl = SIFT_IMPL=SIFTAnatomy SIFTAnatomy_
//tests = SIFTGPU_TEST
//etcConfig = USE_PTR_INC_MALLOC=1
GCC_PREPROCESSOR_DEFINITIONS = \$(inherited) BACKWARD_HAS_UNWIND=1 USE_COMMAND_LINE_ARGS WINDOW_PREFER_GLUT CL_SIFTGPU_ENABLED \$(impl) \$(tests) \$(etcConfig)
InputComesFromHERE
