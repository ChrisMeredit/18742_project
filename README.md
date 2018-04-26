# 18-742 Project: Devon White and Chris Meredith

This project was an attempt to see if removing cache coherence would improve performance enough to justify the loss in accuracy. However, due to some time constraints we ended up looking into removing syncronization instead.

## Getting Started

If you would like to continue this project, you will need a zedboard. Pull the repo ...

### Prerequisites

You will need the RISCV toolchain, the rocket-chip repo, and the GAP benchmark suite.

```
git clone https://github.com/ucb-bar/fpga-zynq.git
```

### Installing

Modifying the nbdcache on the rocketchip repo is optional but first you will need to copy the changes we made into the repo.

Moving over our files

```
cp -r SD_Card_files/fpga-images-zedboard /fpga-zynq repo location/zedboard
```

Also move those files over to the SD card. You will need to reset the zedboard a few times for reasons until you see the orange uart light turn on. At that time you can connect to it by doing.

```
ssh root@192.168.1.5
```
The password is root

See the appenedix with details on how to configure the linux ipconfig to see the zedboard.

## Running the tests

After you make modifications and somehow get the modifications onto the board on linux you will need to run the pr test. You will need to modify the defines to enable the cache changes

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

