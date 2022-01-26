# This nix shell is for converting jupyter notebooks to python scripts with a command like `jupyter nbconvert mynotebook.ipynb --to python` ( https://stackoverflow.com/questions/17077494/how-do-i-convert-a-ipython-notebook-into-a-python-file-via-commandline )

# install nix by running the command on https://nixos.org/download.html . if you get an error then run what it tells you to run in order to fix it (needed for macOS)
# Tip: direnv to keep dependencies for a specific project in Nix
# Run in your terminal: `nix-shell`
# Then run `jupyter notebook`

{ pkgs ? import (builtins.fetchTarball { # https://nixos.wiki/wiki/FAQ/Pinning_Nixpkgs :
  # Descriptive name to make the store path easier to identify
  name = "nixos-unstable-2020-09-03";
  # Commit hash for nixos-unstable as of the date above
  url = "https://github.com/NixOS/nixpkgs/archive/702d1834218e483ab630a3414a47f3a537f94182.tar.gz";
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "1vs08avqidij5mznb475k5qb74dkjvnsd745aix27qcw55rm0pwb";
}) { }}:
with pkgs;

# Tip: `nix-shell -p pkgs.nodejs` an then npm commands work! npm init, npm install (on project, not global). Then do `node2nix -i node-packages.json # creates ./default.nix` and then `nix-shell # nix-shell will look for a default.nix, which above will have generated`. Original source: https://unix.stackexchange.com/questions/379842/how-to-install-npm-packages-in-nixos

let
  # choose a python version here by changing the "38" at the end of this. you can see more versions on https://search.nixos.org by searching for python3
  my-python-packages = python38.withPackages(ps: with ps; [
   #numpy # put a package here by searching its name on https://search.nixos.org
  ]);
in mkShell {
  buildInputs = [
    my-python-packages
    jupyter
                ];
}
