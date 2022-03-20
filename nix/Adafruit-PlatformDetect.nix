{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
, tomli
}:

buildPythonPackage rec {
  pname = "Adafruit-PlatformDetect";
  version = "3.22.0";

  src = fetchPypi {
    sha256 = "sha256-XnB6aSTKRV72WjcXx9jPZ+FGmCNh6dvwiau7WDlyE5M=";
    inherit pname version;
  };
  
  buildInputs = [ (callPackage ./setuptools-scm.nix {})
                  packaging
                  tomli
                ];

  doCheck = false;
}
