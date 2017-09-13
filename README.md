## WunderBar Firmware with mbed OS

This repo uses submodules so execute this after cloning:
```git submodule update --init --recursive```

# Wunderbar mbed OS app only
To build:
1. ```mbed target WUNDERBAR``` (only needed once)
2. ```mbed toolchain GCC_ARM``` (only needed once, GCC_ARM is currently the only supported toolchain)
3. ```mbed compile --profile tools/debug.json -l tools/wunderbar.ld```

To flash:
```JLinkExe -commanderscript flashwb.jlink```

# Wunderbar mbed OS app and Wunderbar DFU bootloader
To build:
1. ```mbed target WUNDERBAR_APP_BIN``` (only needed once)
2. ```mbed toolchain GCC_ARM``` (only needed once, GCC_ARM is currently the only supported toolchain)
3. ```mbed compile --profile tools/profiles/debug.json -l tools/wunderbar.ld```

To flash:
1. Switch Wunderbar to DFU mode (while user button is pressed press & release reset button)
2. ```dfu-util -R -d 15a2:1000 -a 0 -D BUILD/WUNDERBAR_APP_BIN/GCC_ARM/wunderbarv2_application.bin```
3. Reset Wunderbar to APP mode (reset Wunderbar with reset button only)
