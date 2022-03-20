{ lib
, buildPythonPackage
, fetchPypi
, tomli
, packaging
}:

buildPythonPackage rec {
  pname = "setuptools_scm";
  version = "6.4.2";

  src = fetchPypi {
    sha256 = "sha256-aDOsZcbtlxGk1dImb4Akz6B8UzoOVfTBL27/KApanjA=";
    inherit pname version;
  };
  
  propagatedBuildInputs = [ tomli packaging ];

  doCheck = false;
}
