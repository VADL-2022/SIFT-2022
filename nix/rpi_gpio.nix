# Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff

{ stdenv, fetchFromGitHub }:

stdenv.mkDerivation {
  pname = "rpi_gpio";
  version = "0.7.0";
  src = ./nix/RPi.GPIO-0.7.0;
  buildInputs = [];
}
