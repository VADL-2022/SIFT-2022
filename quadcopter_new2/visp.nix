# Based on https://github.com/NixOS/nixpkgs/blob/702d1834218e483ab630a3414a47f3a537f94182/pkgs/development/libraries/libraspberrypi/default.nix

{ pkgs ? import (builtins.fetchTarball { # https://nixos.wiki/wiki/FAQ/Pinning_Nixpkgs :
  # Descriptive name to make the store path easier to identify
  name = "nixos-unstable-2020-09-03";
  # Commit hash for nixos-unstable as of the date above
  url = "https://github.com/NixOS/nixpkgs/archive/702d1834218e483ab630a3414a47f3a537f94182.tar.gz";
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "1vs08avqidij5mznb475k5qb74dkjvnsd745aix27qcw55rm0pwb";
}) { }}:
with pkgs;

stdenv.mkDerivation rec {
  pname = "visp";
  version = "3.4.0";

  src = fetchFromGitHub {
    owner = "lagadic";
    repo = "visp";
    rev = "71a69d037c5e57ed4fa51f02670ec354ddccd198";
    sha256 = "0cnjc7w8ynayj90vlpl13xzm9izd8m5b4cvrq52si9vc6wlm4in5";
  };

  nativeBuildInputs = [ cmake pkg-config ];
  cmakeFlags = [
    (if (stdenv.hostPlatform.isAarch64) then "-DARM64=ON" else "-DARM64=OFF")
  ];

  meta = with lib; {
    description = "Open Source Visual Servoing Platform";
    homepage = "https://github.com/lagadic/visp";
    license = licenses.gpl2;
    platforms = [ "armv6l-linux" "armv7l-linux" "aarch64-linux" "x86_64-linux" "x86_64-darwin" ];
    maintainers = with maintainers; [ sbond75 ];
  };
}
