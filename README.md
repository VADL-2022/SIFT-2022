This is a demo of SIFT for feature matching on frames extracted from a test flight video.

# Installing dependencies

## macOS or Linux: with Nix

1. Install [Nix](https://nixos.org/download.html) (requires macOS or Linux).

## Windows

Not supported unless you manually install OpenCV and a compiler that supports Makefiles

# Running

1. To have all git submodules, ensure you cloned the repo with `git clone --recursive`. If not, run `git submodule update --init --recursive` in the project root to fix it.
2. macOS or Linux: Run `nix-shell` in the project root. Proceed with this shell for the remaining steps.
3. `make -j4` to compile.
4. `./example` to run.
5. When a window comes up, press any key to advance to the next frame and show the matching features.

# Screenshots

![out64](/screenshots/out64.png?raw=true)
![out105](/screenshots/out105.png?raw=true)

# raspi-config, etc. settings

- Enable i2c0 for L3G gyroscope on fore2:
`sudo echo -e "\n""dtparam=i2c_vc=on" >> /boot/config.txt`
  - fore2: EEPROM for pins GPIO 0 and GPIO 1 to prevent voltage from randomly changing on the pins ( https://raspberrypi.stackexchange.com/questions/111312/more-information-about-gpio0-and-gpio1 ):
`sudo echo -e "\n""#EEPROM for pins GPIO 0 and GPIO 1""\n""force_eeprom_read=0" >> /boot/config.txt`
- Enable all of the following in `sudo raspi-config`:
![\[enable all these in sudo raspi-config\] Screen Shot 2022-03-14 at 7.25.31 PM](/documentation/[enable all these in sudo raspi-config] Screen Shot 2022-03-14 at 7.25.31 PM)

- TODO: overclock settings for sift pi's
