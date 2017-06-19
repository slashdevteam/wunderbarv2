## WunderBar Firmware with mbed OS

This repo uses submodules so execute this after cloning:
```git submodule update --init --recursive```

To build:
1. ```mbed target WUNDERBAR``` (only needed once)
2. ```mbed toolchain GCC_ARM``` (only needed once, GCC_ARM is currently the only supported toolchain)
3. ```mbed compile --profile mbed-os/tools/profiles/debug.json```

To flash:
```JLinkExe -commanderscript flashwb.jlink```
