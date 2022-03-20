{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
, tomli
}:

buildPythonPackage rec {
  pname = "adafruit-circuitpython-busdevice";
  version = "5.1.8";

  src = fetchPypi {
    sha256 = "sha256-/JcX6NBTqwFo4E6q5Las4wI7rvHnBGqLcv9U61EpY6o=";
    inherit pname version;
  };
  
  # propagatedBuildInputs prevents infinite looping possibly here. What it does: "consider if every package that contained bash scripts would put bash in `propagatedBuildInputs`. Then I could not use a different version of bash for my user environment than those packages I installed used!" ( https://news.ycombinator.com/item?id=12335846 )
  propagatedBuildInputs = [ (callPackage ./setuptools-scm.nix {})
                            packaging
                            tomli
                            (callPackage ./adafruit-circuitpython-typing.nix {})
                            (callPackage ./Adafruit-Blinka.nix {})
                            (callPackage ./Adafruit-PlatformDetect.nix {})
                            (callPackage ./Adafruit-PureIO.nix {})
                            (callPackage ./pyftdi.nix {})
                          ];

  doCheck = false;
}
