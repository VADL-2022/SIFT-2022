# Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff

{ stdenv, fetchFromGitHub, cmake }:

stdenv.mkDerivation {
  pname = "pigpio";
  version = "79";
  src = fetchFromGitHub {
    owner  = "joan2937";
    repo   = "pigpio";
    rev    = "c33738a320a3e28824af7807edafda440952c05d";
    sha256 = "0wgcy9jvd659s66khrrp5qlhhy27464d1pildrknpdava19b1r37";
  };
  buildInputs = [cmake];
  patches = [ ./pigpio.patch2.patch ];
}

# # Based on https://gist.github.com/taktoa/51af642c37292e6b15d3bd30c4f9c6ff
# 
# { lib, buildPythonPackage, cmake, fetchFromGitHub }:
# 
# buildPythonPackage rec {
#   pname = "pigpio";
#   version = "79";
#   src = fetchFromGitHub {
#     owner  = "joan2937";
#     repo   = "pigpio";
#     rev    = "c33738a320a3e28824af7807edafda440952c05d";
#     sha256 = "0wgcy9jvd659s66khrrp5qlhhy27464d1pildrknpdava19b1r37";
#   };
#   buildInputs = [cmake];
#   patches = [ ./pigpio.patch2.patch ];
# }
# 
