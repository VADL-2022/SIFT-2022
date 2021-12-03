# { pkgs ? import (builtins.fetchTarball { # https://nixos.wiki/wiki/FAQ/Pinning_Nixpkgs :
#   # Descriptive name to make the store path easier to identify
#   name = "nixos-unstable-2020-09-03";
#   # Commit hash for nixos-unstable as of the date above
#   url = "https://github.com/NixOS/nixpkgs/archive/702d1834218e483ab630a3414a47f3a537f94182.tar.gz";
#   # Hash obtained using `nix-prefetch-url --unpack <url>`
#   sha256 = "1vs08avqidij5mznb475k5qb74dkjvnsd745aix27qcw55rm0pwb";
# }) { }}:
# with pkgs;

# pkgs.stdenv.mkDerivation rec {
#   pname = "optick";
#   version = "1.3.1.0";

#   src = fetchFromGithub {
#     owner = "bombomby";
#     repo = "optick";
#     rev = "7f923566d23c6e5aa6cd47bdd7c79bc9453bf01d";
#     sha256 = "0rs9bxxrw4wscf4a8yl776a8g880m5gcm75q06yx2cn3lw2b7v22";
#   };

#   buildInputs = [
#     pkgs.cmake
#   ];

#   configurePhase = ''
#     cmake .
#   '';

#   buildPhase = ''
#     make -j8
#   '';

#   installPhase = ''
#     mkdir -p $out/bin
#     #mv chord $out/bin
#   '';
# }
