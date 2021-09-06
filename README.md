This is a demo of SIFT for feature matching on frames extracted from a test flight video.

# Installing dependencies

## macOS or Linux: with Nix

1. Install [Nix](https://nixos.org/download.html) (requires macOS or Linux).

## Windows

Not supported unless you manually install OpenCV and a compiler that supports Makefiles

# Running

1. macOS or Linux: Run `nix-shell` in the project root. Proceed with this shell for the remaining steps.
2. `make -j4` to compile.
3. `./example` to run.
4. When a window comes up, press any key to advance to the next frame and show the matching features.

# Screenshots

![out64](/screenshots/out64.png?raw=true)
![out105](/screenshots/out105.png?raw=true)
