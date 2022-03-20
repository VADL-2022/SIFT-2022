{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
, tomli
#, pyftdi
}:

buildPythonPackage rec {
  pname = "Adafruit-Blinka";
  version = "7.1.1";

  src = fetchPypi {
    sha256 = "sha256-sV8vuz4q7k1x4f8QenIvdSEpuJif+BATjrLLGzQnrPc=";
    inherit pname version;
  };
  
  buildInputs = [ (callPackage ./setuptools-scm.nix {})
                  packaging
                  tomli
                  (callPackage ./Adafruit-PlatformDetect.nix {})
                  (callPackage ./Adafruit-PureIO.nix {})
                  (callPackage ./pyftdi.nix {}) #pyftdi
                ];

  doCheck = false;
}
