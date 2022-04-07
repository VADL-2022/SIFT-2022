This is an implementation of the VADL Data Processing System (VDPS) and the driver programs (the Primary Mission System and the Driver Script) for feature matching on frames from the camera or video files. See [modules below](##Modules).

# Running for the mission (USE THIS for launch)

1. Set your iPhone's name to "PiRescue" under Settings -> General -> About -> Name. This allows the Pi's to find and conncet to the hotspot on your phone after you enable it in the step below. (After the mission, you can change it back to what it was before.)
   1. If SSH connections are interrupted, use a different iPhone completely, and set the name to `ObersterFuhrerderWermacht` with password `test1567` (or try changing the name and password if your current iPhone to this).
2. Enable personal hotspot on your iPhone under Settings -> Personal Hotspot and then set the password to `test1234`. If it doesn't show up, you don't have that service enabled from your cellular service provider, so use someone else's phone.
3. Connect to that hotspot on your computer.
4. For all 4 of the Raspberry Pi's on the rocket, SSH into each one by using their host name. In other words: `ssh pi@fore1.local` to connect to the first Pi in the fore section; then do `ssh pi@fore2.local`, `ssh pi@sift1.local`, and finally `ssh pi@sift2.local`. And for each Pi run: `tmux`, then `cd VanderbiltRocketTeam`, then `nix-shell`; then, to start the mission sequence, run (and this works for fullscale despite the name!): `bash ./subscale.sh`. The Pi will then wait idly on the pad until takeoff is detected. Press *Ctrl-b* and then *d* to detach from tmux, then *Ctrl-d* to exit that SSH connection to the pi (this leaves tmux running in the background on the Pi). To reconnect, do the same SSH command as used previously and then run `tmux a` to attach to it. If needed, at any point while attached, press *Ctrl-c* multiple times (press it until you see a command-line `$` prompt again) to stop the script/mission.

# Development: installing dependencies

## macOS or Linux: with Nix

1. Install [Nix](https://nixos.org/download.html) (requires macOS or Linux).

## Windows

Not supported unless you manually install OpenCV and a compiler that supports Makefiles

# Development: compiling and running

1. To have all git submodules, ensure you cloned the repo with `git clone --recursive`. If not, run `git submodule update --init --recursive` in the project root to fix it.
2. macOS or Linux: Run `nix-shell` in the project root. Proceed with this shell for the remaining steps.
3. `make -j4 sift_exe_release_commandLine subscale_exe_release` to compile for release, or `make -j4 sift_exe_debug_commandLine subscale_exe_debug` for debug mode.
4. `./sift_exe_release_commandLine --main-mission` to run the mission for release, or `./sift_exe_debug_commandLine --main-mission` for debug. See [this file](/src/main/siftMainCmdLineParser.hpp) for documentation on more command-line arguments. Without `--main-mission` or with `--main-mission-interactive`: when a window comes up, you can press any key to advance to the next frame and show the matching features.

# Screenshots

![out64](/screenshots/out64.png?raw=true)
![out105](/screenshots/out105.png?raw=true)

# Setting up a new Raspberry Pi

- OS to use: `2021-10-30-raspios-bullseye-arm64.img` from [here](https://downloads.raspberrypi.org/raspios_full_armhf/images/raspios_full_armhf-2021-11-08/)
  - For reference, the sha512sum for `2021-10-30-raspios-bullseye-arm64.img` is: `5e8c888d0396eba18684ad7c487dbd873c3491d6215079099f386a42f3e9ab2bdf3ad9c7089fbe82a24fd24a9dd6993401a025caf7200b2ca15db76705458afa`
  - After flashing, create a new file called `ssh` in the `boot` partition of the SD card. On macOS, you can do this as follows:
  
```
cd /Volumes/boot # (macOS)
touch ssh # "If this file exists, ssh will be enabled when the pi is booted." ( https://howchoo.com/g/ote0ywmzywj/how-to-enable-ssh-on-raspbian-without-a-screen )
```

- If you have a keyboard and monitor with mini HDMI cable for the pi: Plug the Raspberry Pi into ethernet and a monitor and keyboard. On the keyboard, press `Ctrl-Alt-t` to open a terminal.
  - Otherwise: see [this](/documentation/Setup%20WiFi%20on%20a%20Pi%20Manually%20using%20wpa_supplicant.conf%20-%20Raspberry%20Pi%20Spy.html)
- Run these:
```
# For WindTunnel repo:
sudo pip3 install adafruit-circuitpython-max31855
sudo pip3 install adafruit-blinka
# Misc:
sudo apt install vim tmux
```
  - Add this to `sudo vim /etc/wpa_supplicant/wpa_supplicant.conf`:
```
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=US

network={
        ssid="PiRescue"
        psk="test1234"
        key_mgmt=WPA-PSK
        priority=3
}
```
 - You can also add any other SSID and password (`psk`) as needed, by making a new `network` block. Higher numbers for `priority` make it connect over other networks of lower priority. If your password or SSID has special characters of some sort, use this and it will print some encoded version which you can use:
    ```
    ORIGINAL_SSID="PiRescue" # Replace this with your iPhone's name
    HEX_SSID="$(echo -n $ORIGINAL_SSID | od -t x1 -A n -w100000 | tr -d ' ')" # Replace `od` with `god` to use GNU od on macOS (install Homebrew first to get this)
    echo "$HEX_SSID"
    ```
    - Put it in like so. The SSID used is the encoded version of "PiRescue":
```
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=US

network={
        ssid=5069526573637565
        psk="test1234"
        key_mgmt=WPA-PSK
        priority=3
}
```
- When modifying this file, it may be somehow reset to its previous contents after a reboot. When rebooting after changing this file, use this to preserve its contents: `sudo service wpa_supplicant stop && sudo reboot`
- If you're on a personal hotspot by now: connect to it on your computer, then `ssh pi@raspberrypi.local` on macOS to SSH into the pi.
  - password is `raspberry` by default
- Install Nix using the commands on [this website](https://nixos.org/download.html)

## `raspi-config`, etc. settings

To get VNC working (not sure if all of these are needed or not) :
- Connect a monitor via HDMI, then choose "start button" (top-left) -> preferences -> screen resolution -> choose one, like 1280x720
- "start button" -> preferences -> raspberry pi configuration -> display -> headless resolution: change to 1280x720
- To get HDMI to be detected when plugged in after booting: `vim /boot/config.txt`: uncomment `hdmi_force_hotplug=1` and then run in vim: `:w !sudo tee %` to write the file, then press enter and then `:q!` command in Vim to force exit.
- `export DISPLAY=:0` and then `lxsession`
- Reboot

- Enable i2c0 for L3G gyroscope on fore2:
`sudo echo -e "\n""dtparam=i2c_vc=on" >> /boot/config.txt`
  - fore2: EEPROM for pins GPIO 0 and GPIO 1 to prevent voltage from randomly changing on the pins ( https://raspberrypi.stackexchange.com/questions/111312/more-information-about-gpio0-and-gpio1 ):
`sudo echo -e "\n""#EEPROM for pins GPIO 0 and GPIO 1""\n""force_eeprom_read=0" >> /boot/config.txt`
- Enable all of the following in `sudo raspi-config` except for Interface->Serial Port->no for login shell, yes for serial port hardware:
![\[enable all these in sudo raspi\-config\] Screen Shot 2022\-03\-14 at 7\.25\.31 PM](/documentation/\[enable%20all%20these%20in%20sudo%20raspi\-config\]%20Screen%20Shot%202022\-03\-14%20at%207\.25\.31%20PM.png?raw=true)
- Enable display: replace `# uncomment to force a specific HDMI mode (this will force VGA)` and the hdmi_group and hdmi_mode areas with this stuff in `/boot/config.txt`:
```
# uncomment to force a specific HDMI mode (this will force VGA)
hdmi_group=2
hdmi_mode=85
```

- Overclock settings for sift pi's for `/boot/config.txt`:
```
over_voltage=6
#2000 is the safe overclock
arm_freq=2000
```

May be necessary if something doesn't work above:
- Enable VNC viewer: `sudo apt-get install realvnc-vnc-server` (should be there by default)

# Software documentation

## To add a data source

- See [SIFT Software Architecture Documentation](/documentation/architecture/SIFT%20Software%20Architecture%20Documentation.pdf)

## Mission sequence

![Driver Script](/documentation/architecture/Mission%20sequence-Page-2.drawio.png?raw=true)
![Video Capture System Script](/documentation/architecture/Mission%20sequence-Page-4.drawio.png?raw=true)

![VADL Data Processing System (VDPS)](/documentation/architecture/Mission%20sequence2-Page-1.drawio.png?raw=true)

- [VDPS](/src/siftMain.cpp) (same as VLS under [Modules](##Modules))

![Primary Mission System](/documentation/architecture/Mission%20sequence2-Page-3.drawio.png?raw=true)

## Modules

![Driver](/documentation/architecture/Modular%20Software%20Architecture2-Page-1.drawio.png?raw=true)

- [Temperature Measurement and Recording System](/WindTunnel/run.py)
- [Driver Script](/subscale.sh) (NOTE: it is called subscale but is actually for fullscale)

![General-Purpose Video Capture System](/documentation/architecture/Modular%20Software%20Architecture2-Page-2.drawio.png?raw=true)

- [General-Purpose Video Capture System](/subscale_driver/videoCapture.py)

![VLS and VMS](/documentation/architecture/Modular%20Software%20Architecture2-Page-3.drawio.png?raw=true)

- [VLS](/src/siftMain.cpp)
- [VMS](/subscale_driver/subscaleMain.cpp)
