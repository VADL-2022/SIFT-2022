{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
, tomli
, typing-extensions
}:

buildPythonPackage rec {
  pname = "adafruit-circuitpython-typing";
  version = "1.3.0";

  src = fetchPypi {
    sha256 = "sha256-Jy16RG0bA/WTr3ukBJZtvNT1huBGY9/yPnOI39j1xBQ=";
    inherit pname version;
  };
  
  # propagatedBuildInputs prevents infinite looping possibly here. What it does: "consider if every package that contained bash scripts would put bash in `propagatedBuildInputs`. Then I could not use a different version of bash for my user environment than those packages I installed used!" ( https://news.ycombinator.com/item?id=12335846 )
  propagatedBuildInputs = [ (callPackage ./setuptools-scm.nix {})
                            packaging
                            tomli
                            typing-extensions
                            #(callPackage ./adafruit-circuitpython-busdevice.nix {}) # CYCLIC DEPENDENCY! infinite loop in nix!!
                          ];

  # Hack to fix infinite loop. It still actually depends on `adafruit-circuitpython-busdevice` but we remove it from the list:
  patchPhase = ''
    substituteInPlace requirements.txt \
      --replace "adafruit-circuitpython-busdevice" ""
    substituteInPlace setup.py \
      --replace "\"adafruit-circuitpython-busdevice\"," ""
  '';

  doCheck = false;
}
