# Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff

{ stdenv, fetchFromGitHub, buildPythonPackage }:

buildPythonPackage {
  pname = "colorzero";
  version = "1.0";
  src = fetchFromGitHub {
    owner  = "waveform80";
    repo   = "colorzero";
    rev    = "d1bf757f59d507817635a09955d2753e3092e7c9";
    sha256 = "1i58kmw5h1s2d1ws8715cz8n1h8962l03f6wh9d6zjm0xc53za98";
  };
}
