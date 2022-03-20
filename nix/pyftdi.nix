# Based on https://github.com/NixOS/nixpkgs/blob/nixos-21.11/pkgs/development/python-modules/pyftdi/default.nix#L29

{ lib
, buildPythonPackage
, fetchPypi
, pyserial
, pythonOlder
, pyusb
}:

buildPythonPackage rec {
  pname = "pyftdi";
  version = "0.54.0";
  disabled = pythonOlder "3.5";

  src = fetchPypi {
    sha256 = "sha256-jfmvIgd9F1M9L5W1CLHYeVmHdifqXcI2kFbpCjtaIy0=";
    inherit pname version;
  };

  # propagatedBuildInputs prevents infinite looping possibly here. What it does: "consider if every package that contained bash scripts would put bash in `propagatedBuildInputs`. Then I could not use a different version of bash for my user environment than those packages I installed used!" ( https://news.ycombinator.com/item?id=12335846 )
  propagatedBuildInputs = [ pyusb #(callPackage ./pyusb.nix {})
                            pyserial #(callPackage ./pyserial.nix {})
                          ];

  # tests requires access to the serial port
  doCheck = false;

  pythonImportsCheck = [ "pyftdi" ];

  meta = with lib; {
    description = "User-space driver for modern FTDI devices";
    longDescription = ''
      PyFtdi aims at providing a user-space driver for popular FTDI devices.
      This includes UART, GPIO and multi-serial protocols (SPI, I2C, JTAG)
      bridges.
    '';
    homepage = "https://github.com/eblot/pyftdi";
    license = licenses.bsd3;
    maintainers = with maintainers; [ fab ];
  };
}
