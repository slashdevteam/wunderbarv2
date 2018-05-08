# WunderBar Firmware with mbed OS

This repo uses submodules, so execute this after cloning:
```git submodule update --init --recursive```

## Requirements
1. [mbed CLI](https://os.mbed.com/docs/v5.8/tools/arm-mbed-cli.html)
2. [ARM GCC toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)

This app requires GCC toolchain - it is set by default, but can be reset with ```mbed toolchain GCC_ARM``` command.

## Wunderbar mbed OS app only
To build:
1. ```mbed target WUNDERBAR``` (only needed once, default target is ```WUNDERBAR_APP_BIN```)
2. ```mbed compile --profile tools/debug.json -l tools/wunderbar.ld``` (```--profile tools/release.json``` can be used to compile release/optimized binary)

To flash:
```JLinkExe -commanderscript flashwb.jlink```

## Wunderbar mbed OS app and Wunderbar DFU bootloader
To build:
1. ```mbed compile --profile tools/debug.json -l tools/wunderbar.ld```

To flash:
1. Switch Wunderbar to DFU mode (press & release reset button while user button is pressed)
2. ```dfu-util -R -d 15a2:1000 -a 0 -D BUILD/WUNDERBAR_APP_BIN/GCC_ARM/wunderbarv2_application.bin```
3. Reset Wunderbar to APP mode (reset Wunderbar with reset button only)
