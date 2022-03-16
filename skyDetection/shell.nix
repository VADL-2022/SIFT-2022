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
  python37m = (pkgs.python37.overrideAttrs (oldAttrs: rec {
    configureFlags = oldAttrs.configureFlags ++ [ "--with-pymalloc" ]; # Enable Pymalloc
  })).override {
    #enableOptimizations = true; # If enabled: longer build time but 10% faster performance.. but the build is no longer reproducible 100% apparently (source: "15.21.2.2. Optimizations" of https://nixos.org/manual/nixpkgs/stable/ )
  };
in
mkShell {
  buildInputs = [
    (python37m.withPackages (p: with p; [
      (callPackage ./nix/imutils.nix {}) #imutils <-- original one depends on opencv3 but we use 4
      opencv4
      numpy
      
      # Fails on python37m:
      # scikitimage
      # pyroma
      # Build for these fails on python37m, but was attempted to fix the above packages failing:
      (callPackage ./nix/scikitimage.nix {Cocoa=pkgs.darwin.apple_sdk.frameworks.Cocoa;}) #scikitimage
      (callPackage ./nix/pyroma.nix {})
    ]))
  ];
}
