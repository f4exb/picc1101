picc1101
========

Connect Raspberry-Pi to CC1101 RF module and play with AX.25/KISS to transmit TCP/IP over the air

Install and basic run
---------------------

# Prerequisites

This has been tested on a Raspberry Pi version 1 B with kernel 3.12.36. Raspberry Pi version 2 with 3.18.y kernel using dtbs has not been working satifactorily so far.

You will need the DMA based SPI driver for BCM2708 found [here][https://github.com/notro/spi-bcm2708.git] 
You will obtain a kernel module that is to be stored as `/lib/modules/$(uname -r)/kernel/drivers/spi/spi-bcm2708.ko` 

You will have to install the WiringPi library found at: 

The process relies heavily on interrupts that must be served in a timely manner. You are advised to reduce the interrupts activity by removing USB connected devices as much as possible.

# Obtain the code

Just clone this repository in a local folder of your choice on the Raspberry Pi

# Compilation

You can compile on the Raspberry Pi v.1 as it doesn't take too much time even on the single core BCM2735. You are advised to activate the -O3 optimization:

`CFLAGS=-O3; make`

The result is the `picc1101` executable in the same directory

# Program options

AX.25/KISS operation
--------------------

# Set up the AX.25/KISS environment

## Kernel modules

You will need to activate the proper options in the `make menuconfig` of your kernel compilation in order to get the `ax25` and `mkiss` modules. It comes by default in most pre-compiled kernels.

Load the modules with `modprobe` command:
  - `sudo modprobe ax25`
  - `sudo modprobe mkiss`

Alternatively you can specify these modules to be loaded at boot time by adding their names in the `/etc/modules` file

## Install AX.25 and KISS software

`sudo apt-get install ...`

## Create your AX.25 interfaces configuration

In `/etc/ax25/axports` you have to add a line with:
`<interface name> <> <> ...`

Example:


## Create a virtual serial link

`sudo mkdir ...`
`socat ...`

## Create the network device using kissattach

`sudo kissattach ...`

## Scripts that will run these commands

In the `scripts` directory you will find:
  - `kissdown.sh`: kills all processes and removes the `ax0` network interface from the system
  - `kissup.sh <IP> <Netmask>`: brings up the `ax0` network interface with IP addres <IP> and net mask <Netmask>

Examples:
  - `./kissdown.sh`
  - `./kissup.sh 10.0..1.3 255.255.255.0`




