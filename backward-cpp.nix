# https://github.com/NixOS/nixpkgs/blob/master/pkgs/development/libraries/backward-cpp/default.nix
{ stdenv, lib, fetchFromGitHub, fixDarwinDylibNames, libunwind }:

# let
#   replaceStr = ''
#                      // try to forward the signal.
#                      raise(info->si_signo);
                 
#                      // terminate the process immediately.
#                      puts("watf? exit");
#                '';
# in
stdenv.mkDerivation rec {
  pname = "backward";
  version = "1.6";

  nativeBuildInputs = [ libunwind fixDarwinDylibNames ];

  src = fetchFromGitHub {
    owner = "bombela";
    repo = "backward-cpp";
    rev = "v${version}";
    sha256 = "1b2h03iwfhcsg8i4f125mlrjf8l1y7qsr2gsbkv0z03i067lykns";
  };

  installPhase = ''
    runHook preInstall
    mkdir -p $out/include
    cp backward.hpp $out/include
    runHook postInstall
  '';

  # postPatch = ''
  #   substituteInPlace backward.hpp \
  #       --replace 'os << "Stack trace (most recent call last)";' ' '
  #   substituteInPlace backward.hpp \
  #       --replace 'os << ":\n";' ' '
  # '';
  
  postPatch = ''
    substituteInPlace backward.hpp \
        --replace '
        // try to forward the signal.
        raise(info->si_signo);

        // terminate the process immediately.
        puts("watf? exit");' ' '
  '';

  meta = with lib; {
    description = "Beautiful stack trace pretty printer for C++";
    homepage = "https://github.com/bombela/backward-cpp";
    license = licenses.mit;
    platforms = platforms.all;
    maintainers = with maintainers; [ cstrahan ];
  };
}
