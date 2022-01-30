# Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff

{ stdenv, fetchFromGitHub, cmake }:

stdenv.mkDerivation {
  pname = "pigpio";
  version = "1.6.2";
  src = fetchFromGitHub {
    owner  = "gpiozero";
    repo   = "gpiozero";
    rev    = "e55c678ff0dfe0b34c2b0a1683869dbc7e0086d2";
    sha256 = "0wgcy9jvd659s66khrrp5qlhhy27464d1pildrknpdava29b1r37";
  };
}
