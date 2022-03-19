{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
}:

buildPythonPackage rec {
  pname = "adafruit-circuitpython-gps";
  version = "3.9.7";

  src = fetchPypi {
    sha256 = "sha256-kkaA9WX0+fPLBWQ9IrEBKgVXvusz+7steM9BVSGWxEc=";
    inherit pname version;
  };
  
  buildInputs = [ (callPackage ./setuptools-scm.nix {}) packaging ];

  doCheck = false;
}
