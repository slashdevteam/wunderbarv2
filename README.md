## WunderBar Firmware with mbed OS

This repo uses submodules so execute this after cloning:
```git submodule update --init --recursive```

# Only Wunderbar mbed OS app
To build:
1. ```mbed target WUNDERBAR``` (only needed once)
2. ```mbed toolchain GCC_ARM``` (only needed once, GCC_ARM is currently the only supported toolchain)
3. ```mbed compile --profile mbed-os/tools/profiles/debug.json```

To flash:
```JLinkExe -commanderscript flashwb.jlink```

# Wunderbar mbed OS app and Wunderbar DFU bootloader
To build:
1. ```mbed target WUNDERBAR_APP_BIN``` (only needed once)
2. ```mbed toolchain GCC_ARM``` (only needed once, GCC_ARM is currently the only supported toolchain)
3. ```mbed compile --profile mbed-os/tools/profiles/debug.json```

To flash:
1. Switch Wunderbar to DFU mode (reset Wunderbar with user button pressed)
2. ```dfu-util -R -d 15a2:1000 -a 0 -D BUILD/WUNDERBAR_APP_BIN/GCC_ARM/wunderbarv2_application.bi```
3. Reset Wunderbar to APP mode (reset Wunderbar normally)
