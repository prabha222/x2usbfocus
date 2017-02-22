# X2USBFocus

This project is a X2 driver for USBFocus tested on Ubuntu 16.04 64bit. X2 drivers are used in TheSkyX software. This driver allows you to use USBFocus device in TheSkyX for Linux.

# How to Install

Download the Release, it contains 3 files needs to copy in the TheSkyX folder. 
* Download binary file (libX2USBFocus.zip).
* Download USBFocus.ui
* Download FocuserList USBFocus.txt

"FocuserList USBFocus.txt" should be copied in TheSkyX Installation Folder/Contents/Resources/Common/Miscellaneous Files folder
"USBFocus.ui" should be copied in TheSkyX Installation Folder/Contents/Resources/Common/Plugins/FocuserPlugins folder
"libX2USBFocus.so" should be copied in TheSkyX Installation Folder/Contents/Resources/Common/Plugins/FocuserPlugins folder

usbfocus is a cdc acm class device, on connecting cdc_acm driver will create /dev/ttyACM0 interface for communication.

TheSkyX uses a set of "pre-defined" serial sym-links generated through udev-rules which is also hardcoded in the main TheSkyX binary and unfortunately will not detect /dev/ttyACM0 to use.

Workaround for this is to update "/etc/udev/rules.d/10-SoftBisque.rules" file.
In my case i do not have any FTDI serial devices, so i modified existing entry with usbfocus vendor(0x0461) and device(0x0033) id.

\#ACTION=="add", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666", SYMLINK+="FTDIusb"
ACTION=="add", ATTRS{idVendor}=="0461", ATTRS{idProduct}=="0033", MODE="0666", SYMLINK+="FTDIusb"

if you use FTDIusb serial device, you could pick "Prolific",etc
After adding this new entry, udev needs to be reloaded to use updated symlink.

"sudo udevadm control --reload-rules"

To verify, do "ls -l /dev/FTDIusb" and it should show the link to /dev/ttyACM0

Now you can select "usbfo.us" from TheSkyX and in settings pick "/dev/FTDIusb" serial device.

Occasionally i noticed focuser might not respond while "Connect", if this happen close TheSkyX and 
reload cdc_acm driver to fix. 

1) close TheSkyX
2) Disconnect usb cable
3) "sudo rmmod cdc_acm"
4) "sudo modprobe cdc_acm"

Build instructions
1) Install qt5-make, qt5-default
sudo apt install qt5-make
sudo apt install qt5-default

2) Open terminal and change directory to x2usbfocus directory
qmake && make

if everything went well, you should see libx2usbfocus.so generated.
copy .so to theskyx focoserplugins folder from above instructions.

Enjoy! 
