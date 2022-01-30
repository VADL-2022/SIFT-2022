# Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff

{ stdenv, fetchFromGitHub, buildPythonPackage, callPackage }:

buildPythonPackage {
  pname = "gpiozero";
  version = "1.6.2";
  src = fetchFromGitHub {
    owner  = "gpiozero";
    repo   = "gpiozero";
    rev    = "e55c678ff0dfe0b34c2b0a1683869dbc7e0086d2";
    sha256 = "18hiljf86jncv636rc80ik0s3xsrnq1g2phfc6ild52k6gf5qqbn";
  };
  doCheck = false;
  buildInputs = [ (callPackage ./colorzero.nix {buildPythonPackage=buildPythonPackage;}) ];
}
