{ lib
, buildPythonPackage
, fetchPypi
, callPackage
, packaging
, tomli
}:

buildPythonPackage rec {
  pname = "Adafruit_PureIO";
  version = "1.1.9";

  src = fetchPypi {
    sha256 = "sha256-LK8i+wfH93HYMmfzMads3jFHI/iEqVcOpvdocwyHqHk=";
    inherit pname version;
  };
  
  # propagatedBuildInputs prevents infinite looping possibly here. What it does: "consider if every package that contained bash scripts would put bash in `propagatedBuildInputs`. Then I could not use a different version of bash for my user environment than those packages I installed used!" ( https://news.ycombinator.com/item?id=12335846 )
  propagatedBuildInputs = [ (callPackage ./setuptools-scm.nix {})
                            packaging
                            tomli
                          ];

  doCheck = false;
}
