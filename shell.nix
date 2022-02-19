# Tip: direnv to keep dependencies for a specific project in Nix
# Run: nix-shell

{ useGtk ? true,
  pkgs ? import (builtins.fetchTarball { # https://nixos.wiki/wiki/FAQ/Pinning_Nixpkgs :
  # Descriptive name to make the store path easier to identify
  name = "nixos-unstable-2020-09-03";
  # Commit hash for nixos-unstable as of the date above
  url = "https://github.com/NixOS/nixpkgs/archive/702d1834218e483ab630a3414a47f3a537f94182.tar.gz";
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "1vs08avqidij5mznb475k5qb74dkjvnsd745aix27qcw55rm0pwb";
}) { }}:
with pkgs;

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
  python37m = (pkgs.python37.overrideAttrs (oldAttrs: rec {
    configureFlags = oldAttrs.configureFlags ++ [ "--with-pymalloc" ]; # Enable Pymalloc
  })).override {
    #enableOptimizations = true; # If enabled: longer build time but 10% faster performance.. but the build is no longer reproducible 100% apparently (source: "15.21.2.2. Optimizations" of https://nixos.org/manual/nixpkgs/stable/ )
  };
  # #
in
mkShell {
  buildInputs = [ #my-python-packages
                ] ++ (lib.optional (stdenv.hostPlatform.isMacOS || !useGtk) [ opencv4 ])
  ++ (lib.optional (stdenv.hostPlatform.isLinux && useGtk) [ (python37Packages.opencv4.override { enableGtk2 = true; })
                                                                 opencvGtk
                                                               ]) ++ [
    clang_12 # Need >= clang 10 to fix fast-math bug (when using -Ofast) ( https://bugzilla.redhat.com/show_bug.cgi?id=1803203 )
    pkgconfig libpng
    ] ++ (lib.optional (stdenv.hostPlatform.isLinux) [ lldb x11 ]) ++ [

      #] ++ (builtins.exec ["bash" "-c", "xcrun --show-sdk-version"] >= 11.3 [] # no jemalloc for Latif due to newer macOS SDK impurity causing jemalloc build to fail..
      #else: [
      jemalloc #] 
      
    #bear # Optional, for generating emacs compile_commands.json

    # For Python interop #
    python37Packages.pybind11
    # #
    
    # For stack traces #
    (callPackage ./backward-cpp.nix {}) # https://github.com/bombela/backward-cpp
    ] ++ (lib.optional (stdenv.hostPlatform.isMacOS) libunwind_modded) ++ (lib.optional (stdenv.hostPlatform.isLinux) libunwind) ++ [
    # #
    
      # VADL2022 "library" #
      coreutils
    (python37m.withPackages (p: with p; [
      (callPackage ./pyserial_nix/pyserial.nix {}) #pyserial # https://pyserial.readthedocs.io/en/latest/
      opencv4
      #numpy
      
      # For Python interop #
      pybind11
      # #
      
      (lib.optional stdenv.hostPlatform.isMacOS opencv4)
      (lib.optional useGtk (toPythonModule (pkgs.opencv4.override { enableGTK2 = true; enablePython = true; pythonPackages = python37Packages; }))) # Temp hack
      numpy
      #matplotlib

      #scipy

      # For LIS331HH IMU
    ] ++ (lib.optional (stdenv.hostPlatform.isLinux) [ (callPackage ./nix/smbus2.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
                                                    (callPackage ./nix/rpi_gpio.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
                                                    (callPackage ./nix/gpiozero.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
                                                    (callPackage ./nix/colorzero.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
                                                    (callPackage ./nix/pigpio_python.nix {buildPythonPackage=python37m.pkgs.buildPythonPackage;})
                                                     ]) ++ [
    ]))
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
