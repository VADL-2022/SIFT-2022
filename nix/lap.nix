# https://pypi.org/project/lap/

{ stdenv, lib, fetchPypi, buildPythonPackage, numpy }:

buildPythonPackage rec {
  pname = "lap";
  version="0.4.0";

  src = fetchPypi {
    inherit pname version;
    sha256 = "0fqfxpq4jg9h4wxjw540gjmvfg1ccc1nssk7i9njg7qfdybxknn4";
  };

  propagatedBuildInputs = [ numpy ];

  doCheck = false;
}
