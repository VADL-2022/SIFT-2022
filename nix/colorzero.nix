# Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff

{ stdenv, fetchFromGitHub, buildPythonPackage }:

buildPythonPackage {
  pname = "colorzero";
  version = "1.0";
  src = fetchFromGitHub {
    owner  = "waveform80";
    repo   = "colorzero";
    rev    = "d1bf757f59d507817635a09955d2753e3092e7c9";
    sha256 = "18hiljf86jncv636rc80ik0s3xsrnq1g2phac6ild52k6gf5qqbn";
  };
}
