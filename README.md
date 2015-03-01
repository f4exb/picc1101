picc1101
========

Connect Raspberry-Pi to CC1101 RF module and play with AX.25/KISS to transmit TCP/IP over the air

# Introduction

The aim of this program is to connect a RF module based on the Texas Instruments (Chipcon) chip CC1101 to a Raspberry-Pi host machine. The CC1101 chip is a OOK/2-FSK/4-FSK/MSK/GFSK low power (~10dBm) digital transceiver working in the 315, 433 and 868 MHz ISM bands. The 433 MHz band also happens to cover the 70cm Amateur Radio band and the major drive of this work is to use these modules as a modern better alternative to the legacy TNCs working in 1200 baud FM AFSK or 9600 baud G3RUH true 2-FSK modulation at best. Using the Linux native AX.25 and KISS interface to the TNCs it is then possible to route TCP/IP traffic using these modules offering the possibility to connect to the Amateur Radio private IP network known as [Hamnet](http://hamnetdb.net/).

Another opportunity is the direct transmission of a Transport Stream to carry low rate live video and this will be studied later.

These RF modules are available from a variety of sellers on eBay (search for words 'CC1101' and '433 MHz') and the Raspberry-Pi doesn't need to be introduced any further!

The CC1101 chip implements preamble, sync word, CRC, data whitening and FEC using convolutive coding natively. It is a very nice little cheap chip for our purpose. It has all the necessary features to cover the OSI layer 1 (physical).

The CC1101 chip is interfaced using a SPI bus that is implemented natively on the Raspberry-PI and can be accessed through the `spidev` library. In addition two GPIOs must be used to support the handling of the CC1101 Rx and Tx FIFOs. For convenience GPIO-24 and GPIO-25 close to the SPI bus on the Raspberry-Pi are chosen to be conencted to the GDO0 and GDO2 lines of the CC1101 respectively. The WiringPi library is used to support the GPIO interrupt handling.

The CC1101 data sheet is available [here](www.ti.com/lit/ds/symlink/cc1101.pdf).

# Disclaimer

You are supposed to use the CC1101 modules and this software sensibly. Please check your local radio spectrum regulations. For Amateur Radio use you should have a valid Amateur Radio licence with a callsign and transmit in the bands and conditions granted by your local regulations also please try to respect the IARU band plan. 

# Install and basic run

## Prerequisites

This has been tested on a Raspberry Pi version 1 B with kernel 3.12.36. Raspberry Pi version 2 with 3.18 kernels using dtbs has not been working satifactorily so far. Version 2 is very new (in March 2015) and improvements are expected concerning SPI and GPIO handling and it will probably work one day on the version 2 as well. Anyway version 1 has enough computing power for our purpose.

You will need the DMA based SPI driver for BCM2708 found [here](https://github.com/notro/spi-bcm2708.git) After successful compilation you will obtain a kernel module that is to be stored as `/lib/modules/$(uname -r)/kernel/drivers/spi/spi-bcm2708.ko` 

You will have to download and install the WiringPi library found [here](http://wiringpi.com/) 

The process relies heavily on interrupts that must be served in a timely manner. You are advised to reduce the interrupts activity by removing USB connected devices as much as possible.

## Obtain the code

Just clone this repository in a local folder of your choice on the Raspberry Pi

## Compilation

You can compile on the Raspberry Pi v.1 as it doesn't take too much time even on the single core BCM2735. You are advised to activate the -O3 optimization:
  - `CFLAGS=-O3; make`

The result is the `picc1101` executable in the same directory

## Program options

# AX.25/KISS operation

## Set up the AX.25/KISS environment

### Kernel modules

You will need to activate the proper options in the `make menuconfig` of your kernel compilation in order to get the `ax25` and `mkiss` modules. It comes by default in most pre-compiled kernels.

Load the modules with `modprobe` command:
  - `sudo modprobe ax25`
  - `sudo modprobe mkiss`

Alternatively you can specify these modules to be loaded at boot time by adding their names in the `/etc/modules` file

### Install AX.25 and KISS software

`sudo apt-get install ...`

### Create your AX.25 interfaces configuration

In `/etc/ax25/axports` you have to add a line with:
`<interface name> <> <> ...`

Example:


### Create a virtual serial link

`sudo mkdir ...`
`socat ...`

### Create the network device using kissattach

`sudo kissattach ...`

### Scripts that will run these commands

In the `scripts` directory you will find:
  - `kissdown.sh`: kills all processes and removes the `ax0` network interface from the system
  - `kissup.sh <IP> <Netmask>`: brings up the `ax0` network interface with IP addres <IP> and net mask <Netmask>

Examples:
  - `./kissdown.sh`
  - `./kissup.sh 10.0..1.3 255.255.255.0`

## Run the program


