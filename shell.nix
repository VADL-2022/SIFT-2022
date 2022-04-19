# Tip: direnv to keep dependencies for a specific project in Nix
# Run: nix-shell

{ useGtk ? true,
  pkgs ? (builtins.fetchTarball { # https://nixos.wiki/wiki/FAQ/Pinning_Nixpkgs :
  # Descriptive name to make the store path easier to identify
  name = "nixos-unstable-2020-09-03";
  # Commit hash for nixos-unstable as of the date above
  url = "https://github.com/NixOS/nixpkgs/archive/702d1834218e483ab630a3414a47f3a537f94182.tar.gz";
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "1vs08avqidij5mznb475k5qb74dkjvnsd745aix27qcw55rm0pwb";
})}:
#with pkgs;

# Apply overlay
let
  myOverlay = (self: super: { #(final: prev: {
    #python37 = super.python37.overrideAttrs (oldAttrs: rec {
    python37 = super.python37.override {
      packageOverrides = self: super: {
        pyroma = self.pkgs.python37Packages.callPackage ./skyDetection/nix/pyroma.nix {};
        pillow = self.pkgs.python37Packages.callPackage ./skyDetection/nix/pillow.nix {};
        matplotlib = self.pkgs.python37Packages.callPackage ./skyDetection/nix/matplotlib.nix {Cocoa=self.pkgs.darwin.apple_sdk.frameworks.Cocoa;};
      };
    };
    # export NIXPKGS_ALLOW_UNFREE=1
    # opencv4 = super.opencv4.override {
    #   enableUnfree = true;
    # };
  }); # this is two lambdas (curried + nested)
  nixpkgs = import pkgs {};
  finalPkgs = import pkgs {
    system = if false then "x86_64-darwin" else builtins.currentSystem; # For M1 Mac to work
    # Identity: overlays = [];
    overlays = [ myOverlay ];
  };
in
with finalPkgs;

let
  opencvGtk = opencv4.override (old : { enableGtk2 = true; }); # https://stackoverflow.com/questions/40667313/how-to-get-opencv-to-work-in-nix , https://github.com/NixOS/nixpkgs/blob/master/pkgs/development/libraries/opencv/default.nix
  # my-python-packages = python39.withPackages(ps: with ps; [
  #   (lib.optional stdenv.hostPlatform.isMacOS opencv4)
  #   (lib.optional useGtk (toPythonModule (pkgs.opencv4.override { enableGTK2 = true; enablePython = true; pythonPackages = python39Packages; }))) # Temp hack
  #   numpy
  #   matplotlib
  # ]);

  # Actual package is https://github.com/NixOS/nixpkgs/blob/nixos-21.05/pkgs/development/compilers/llvm/7/libunwind/default.nix#L45
  libunwind_modded = llvmPackages.libunwind.overrideAttrs (oldAttrs: rec {
      nativeBuildInputs = oldAttrs.nativeBuildInputs ++ [ fixDarwinDylibNames ];
  });

  # VADL2022 "library" #
  python37m = (python37.overrideAttrs (oldAttrs: rec {
    configureFlags = oldAttrs.configureFlags ++ [ "--with-pymalloc" ]; # Enable Pymalloc
  })).override {
    #enableOptimizations = true; # If enabled: longer build time but 10% faster performance.. but the build is no longer reproducible 100% apparently (source: "15.21.2.2. Optimizations" of https://nixos.org/manual/nixpkgs/stable/ )
  };
  # #
  #pyroma_ = (callPackage ./skyDetection/nix/pyroma.nix {});
  #pillow_ = (callPackage ./skyDetection/nix/pillow.nix {pyroma=pyroma_;});
  #pillow_ = (callPackage ./skyDetection/nix/pillow.nix {});
  #matplotlib_ = (callPackage ./skyDetection/nix/matplotlib.nix {Cocoa=darwin.apple_sdk.frameworks.Cocoa; fetchPypi=python37m.fetchPypi; buildPythonPackage=python37m.buildPythonPackage; isPy3k=python37m.isPy3k;});
  #matplotlib_ = (callPackage python37Packages.matplotlib {pyroma=pyroma_; pillow=pillow_;});
in
mkShell {
  # propagatedBuildInputs = [
  #   (callPackage ./skyDetection/nix/pyroma.nix {})
  #   (callPackage ./skyDetection/nix/pillow.nix {})
    
  #   # keras-preprocessing.override (old : {
      
  #   # })
  # ];
  buildInputs = [ #my-python-packages
                # ] ++ (lib.optionals (stdenv.hostPlatform.isMacOS || !useGtk) [ opencv4 ])
  # ++ (lib.optionals (stdenv.hostPlatform.isLinux && useGtk) [
  #   #(python37Packages.opencv4.override { enableGtk2 = true; })
  #   opencvGtk
  # ]) ++ [
    clang_12 # Need >= clang 10 to fix fast-math bug (when using -Ofast) ( https://bugzilla.redhat.com/show_bug.cgi?id=1803203 )
    pkgconfig libpng
    ] ++ (lib.optionals (stdenv.hostPlatform.isLinux) [ lldb x11 ]) ++ [

      #] ++ (builtins.exec ["bash" "-c", "xcrun --show-sdk-version"] >= 11.3 [] # no jemalloc for Latif due to newer macOS SDK impurity causing jemalloc build to fail..
      #else: [
      #jemalloc #] 
      
    #bear # Optional, for generating emacs compile_commands.json

    # For Python interop #
    python37Packages.pybind11
    # #
    
    # For stack traces #
    (callPackage ./backward-cpp.nix {}) # https://github.com/bombela/backward-cpp
    ] ++ (lib.optional (stdenv.hostPlatform.isMacOS) libunwind_modded) ++ (lib.optional (stdenv.hostPlatform.isLinux) libunwind) ++ [
    # #

    #lmdb
   
    ] ++ (lib.optional (stdenv.hostPlatform.isLinux) [ 
      gpsd # For gps
    ]) ++ [

    # VADL2022 "library" #
    coreutils
    (python37m.withPackages (p: with p; ([
      pyserial #(callPackage ./pyserial_nix/pyserial.nix {}) #pyserial # https://pyserial.readthedocs.io/en/latest/
      #opencv4
      #numpy
  
      ] ++ (lib.optional (stdenv.hostPlatform.isLinux) [
        gps3
      ]) ++ [
    
      # For Python interop #
      pybind11
      # #
      
    ] ++ (lib.optionals stdenv.hostPlatform.isMacOS [ opencv4 ]) ++
    (lib.optionals (useGtk && stdenv.hostPlatform.isLinux) [ (opencv4.override { enableGtk2 = true; enablePython = true; pythonPackages = python37Packages; }) ]) ++ [ # Temp hack
      numpy

      # For https://github.com/yzhq97/cnn-registration #
      scipy
      #matplotlib_
      (callPackage ./skyDetection/nix/matplotlib.nix {Cocoa=darwin.apple_sdk.frameworks.Cocoa;})
      #(callPackage ./skyDetection/nix/matplotlib.nix {})
      #(callPackage tensorflow {matplotlib=matplotlib_; pyroma=pyroma_;})
      #tensorflow #tensorflow.override (old : {nativeBuildInputs.matplotlib=(callPackage ./skyDetection/nix/matplotlib.nix {Cocoa=darwin.apple_sdk.frameworks.Cocoa;}); nativeBuildInputs.pyroma=pyroma_;}) # nix show-derivation /nix/store/95jcq26lvz2fijxndja6yp2dpq4mi293-python3.7-tensorflow-2.4.2.drv
      #(callPackage ./nix/tensorflow.nix {keras=Keras; protobuf-python=protobuf; flatbuffers-python=flatbuffers; flatbuffers-core=finalPkgs.flatbuffers; lmdb-core=finalPkgs.lmdb; protobuf-core=finalPkgs.protobuf; Foundation=darwin.apple_sdk.frameworks.Foundation; Security=darwin.apple_sdk.frameworks.Security; cctools=darwin.cctools;})
      (callPackage ./nix/lap.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
      # #

      pandas

      #scipy

      # For LIS331HH IMU
      ] ++ (lib.optionals (stdenv.hostPlatform.isLinux) [
        (callPackage ./nix/smbus2.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
        (callPackage ./nix/rpi_gpio.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
        (callPackage ./nix/gpiozero.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
        (callPackage ./nix/colorzero.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
        (callPackage ./nix/pigpio_python.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
        (callPackage ./nix/adafruit-circuitpython-gps.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;}) ]) ++ [
    ])))
    ] ++ (lib.optional (stdenv.hostPlatform.isLinux) [ (callPackage ./nix/pigpio.nix {}) ]) ++ [
    #] ++ (lib.optional (stdenv.hostPlatform.isLinux) [ (callPackage ./nix/pigpio.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;}) ]) ++ [
    libusb1
    # #

    # For SiftGPU #
    glew
    libGL
    freeglut
    libdevil
    opencl-headers
    #opencl-clhpp
    # #

    #] ++ (lib.optional (stdenv.hostPlatform.isLinux) [ sudo ]) ++ [

    llvmPackages.libstdcxxClang
    #ginac # https://www.ginac.de/tutorial/#A-comparison-with-other-CAS
    
    #(callPackage ./webscraping.nix {})
  ]; # Note: for macos need this: write this into the path indicated:
  # b) For `nix-env`, `nix-build`, `nix-shell` or any other Nix command you can add
  #   { allowUnsupportedSystem = true; }
  # to ~/.config/nixpkgs/config.nix.
  # ^^^^^^^^^^^^^^^^^^ This doesn't work, use `brew install cartr/qt4/pyqt` instead.
  
  shellHook = ''
    export INCLUDE_PATHS_FROM_CFLAGS=$(./makeIncludePathsFromCFlags.sh)
  '';
}
