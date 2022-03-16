# https://github.com/NixOS/nixpkgs/blob/885e42e78f0120f904924eff5c794672dd9e5a1a/pkgs/development/python-modules/pyroma/default.nix , https://github.com/NixOS/nixpkgs/pull/137895/files

{ lib, buildPythonPackage, fetchPypi
, docutils, pygments, setuptools, requests
,   fetchpatch
, pythonOlder
}:

buildPythonPackage rec {
  pname = "pyroma";
  version = "3.2";

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-+MGB4NXSkvEXka/Bj30CGKg8hc9k1vj7FXHOnSmiTko=";
  };

  patches = lib.optionals (pythonOlder "3.8") [
    # upstream fix for python-3.7 test failures:
    #  https://github.com/NixOS/nixpkgs/issues/136901
    (fetchpatch {
      name = "fix-py37-tests.patch";
      url = "https://github.com/regebro/pyroma/commit/d30bc41da7d17a0737eb8ad2267457eff4cac82c.patch";
      sha256 = "003vvp360cc05kas6bm6pdd6wgw4x3399sprr6pl7f654s730psq";
      excludes = [ "HISTORY.txt" ];
    })
];

  propagatedBuildInputs = [ docutils pygments setuptools requests ];

  # Tests require network access
  doCheck = false;

  meta = with lib; {
    description = "Test your project's packaging friendliness";
    homepage = "https://github.com/regebro/pyroma";
    license = licenses.mit;
  };
}
