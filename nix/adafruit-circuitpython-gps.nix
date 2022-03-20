{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
, tomli
, pyserial
}:

buildPythonPackage rec {
  pname = "adafruit-circuitpython-gps";
  version = "3.9.7";

  src = fetchPypi {
    sha256 = "sha256-kkaA9WX0+fPLBWQ9IrEBKgVXvusz+7steM9BVSGWxEc=";
    inherit pname version;
  };
  
  propagatedBuildInputs = [ (callPackage ./setuptools-scm.nix {})
                            packaging
                            tomli
                            (callPackage ./Adafruit-Blinka.nix {})
                            pyserial
                            #(callPackage ./pyserial.nix {})
                            (callPackage ./adafruit-circuitpython-busdevice.nix {})
                          ];

  doCheck = false;
}
