picc1101
========

Connect Raspberry-Pi to CC1101 RF module and play with AX.25/KISS to transmit TCP/IP over the air.

- [picc1101](#picc1101)
- [Introduction](#introduction)
- [Disclaimer](#disclaimer)
- [Installation and basic usage](#installation-and-basic-usage)
  - [Prerequisites](#prerequisites)
  - [Obtain the code](#obtain-the-code)
  - [Compilation](#compilation)
  - [Run test programs](#run-test-programs)
  - [Process priority](#process-priority)
    - [Specify a higher priority at startup](#specify-a-higher-priority-at-startup)
    - [Engage the "real time" priority](#engage-the-real-time-priority)
  - [Program options](#program-options)
  - [Detailed options](#detailed-options)
    - [Verbosity level (-v)](#verbosity-level--v)
    - [Radio interface speeds (-R)](#radio-interface-speeds--r)
    - [Modulations (-M)](#modulations--m)
    - [Test routines (-t)](#test-routines--t)
- [AX.25/KISS operation](#ax25kiss-operation)
  - [Set up the AX.25/KISS environment](#set-up-the-ax25kiss-environment)
    - [Kernel modules](#kernel-modules)
    - [Install AX.25 and KISS software](#install-ax25-and-kiss-software)
    - [Create your AX.25 interfaces configuration](#create-your-ax25-interfaces-configuration)
    - [Create a virtual serial link](#create-a-virtual-serial-link)
    - [Create the network device using kissattach](#create-the-network-device-using-kissattach)
    - [Scripts that will run these commands](#scripts-that-will-run-these-commands)
  - [Run the program](#run-the-program)
  - [Relay the KST chat](#relay-the-kst-chat)
- [Details of the design](#details-of-the-design)
  - [Multiple block handling](#multiple-block-handling)
  - [Mitigate AX.25/KISS spurious packet retransmissions](#mitigate-ax25kiss-spurious-packet-retransmissions)

# Introduction
The aim of this program is to connect a RF module based on the Texas Instruments (Chipcon) chip CC1101 to a Raspberry-Pi host machine. The CC1101 chip is a OOK/2-FSK/4-FSK/MSK/GFSK low power (~10dBm) digital transceiver working in the 315, 433 and 868 MHz ISM bands. The 433 MHz band also happens to cover the 70cm Amateur Radio band and the major drive of this work is to use these modules as a modern better alternative to the legacy [Terminal Node Controllers](http://en.wikipedia.org/wiki/Terminal_node_controller) or TNCs working in 1200 baud FM AFSK or 9600 baud G3RUH true 2-FSK modulation at best. Using the Linux native AX.25 and KISS interface to the TNCs it is then possible to route TCP/IP traffic using these modules offering the possibility to connect to the Amateur Radio private IP network known as [Hamnet](http://hamnetdb.net/).

Another opportunity is the direct transmission of a Transport Stream to carry low rate live video and this will be studied later.

These RF modules are available from a variety of sellers on eBay. Search for words `CC1101` and `433 MHz`. The Raspberry-Pi doesn't need to be introduced any further. If you are reading these lines you probably know what it is!

The CC1101 chip implements preamble, sync word, CRC, data whitening and FEC using convolutive coding natively. It is a very nice little cheap chip for our purpose. It has all the necessary features to cover the OSI layer 1 (physical). Its advertised speed ranges from 600 to 500000 Baud (300000 in 4-FSK) but it can go as low as 50 baud however details on performance at this speed have not been investigated. Yet the program offers this possibility.

The CC1101 chip is interfaced using a SPI bus that is implemented natively on the Raspberry-PI and can be accessed through the `spidev` library. In addition two GPIOs must be used to support the handling of the CC1101 Rx and Tx FIFOs. For convenience GPIO-24 and GPIO-25 close to the SPI bus on the Raspberry-Pi are chosen to be connected to the GDO0 and GDO2 lines of the CC1101 respectively. The WiringPi library is used to support the GPIO interrupt handling.

The CC1101 data sheet is available [here](www.ti.com/lit/ds/symlink/cc1101.pdf).

# Disclaimer
You are supposed to use the CC1101 modules and this software sensibly. Please check your local radio spectrum regulations. 

For Amateur Radio use you should have a valid Amateur Radio licence with a callsign and transmit in the bands and conditions granted by your local regulations also please try to respect the IARU band plan. In most if not all countries you are not allowed to transmit encrypted data so please do not route SSL traffic like `https` or `ssh`. Use plain `http` or `telnet` instead.

# Installation and basic usage
## Prerequisites
This has been tested successfully on a Raspberry Pi version 1 B with kernel 3.12.36. Raspberry Pi version 2 with a 3.18 kernel does not work.

For best performance you will need the DMA based SPI driver for BCM2708 found [here](https://github.com/notro/spi-bcm2708.git) After successful compilation you will obtain a kernel module that is to be stored as `/lib/modules/$(uname -r)/kernel/drivers/spi/spi-bcm2708.ko` 

You will have to download and install the WiringPi library found [here](http://wiringpi.com/) 

The process relies heavily on interrupts that must be served in a timely manner. You are advised to reduce the interrupts activity by removing USB connected devices as much as possible.

## Obtain the code
Just clone this repository in a local folder of your choice on the Raspberry Pi

## Compilation
You can compile on the Raspberry Pi v.1 as it doesn't take too much time even on the single core BCM2735. You are advised to activate the -O3 optimization:
  - `CFLAGS=-O3 make`

The result is the `picc1101` executable in the same directory

## Run test programs
On the sending side:
  - `sudo ./picc1101 -v1 -B 9600 -P 252 -R7 -M4 -W -l15 -t2 -n5`

On the receiving side:
  - `sudo ./picc1101 -v1 -B 9600 -P 252 -R7 -M4 -W -l15 -t4 -n5`

This will send 5 blocks of 252 bytes at 9600 Baud using GFSK modulation and receive them at the other end. The block will contain the default test phrase `Hello, World!`.

Note that you have to be super user to execute the program.

## Process priority
You may experience better behaviour (less timeouts) depending on the speed of the link when raising the prioriry of the process. Interrupts are already served with high priority (-56) with the WiringPi library. The main process may need a little boost as well though

### Specify a higher priority at startup
You can use the `nice` utility: `sudo nice -n -20 ./picc1101 options...` 
This will set the priority to 0 and is the minimum you can obtain with the `nice` commmand. The lower the priority figure the higher the actual priority. 

### Engage the "real time" priority
You can use option -T of the program to get an even lower priority of -2 for a so called "real time" scheduling. This is not real time actually but will push the priority figure into the negative numbers. It has been implemented with the WiringPi piHiPri method and -2 is the practical lowest figure possible before entering into bad behaviour that might make a cold reboot necessary. Note that this is the same priority as the watchdog.

## Program options
 <pre><code>
  -B, --tnc-serial-speed=SERIAL_SPEED
                             TNC Serial speed in Bauds (default : 9600)
  -d, --spi-device=SPI_DEVICE   SPI device, (default : /dev/spidev0.0)
  -D, --tnc-serial-device=SERIAL_DEVICE
                             TNC Serial device, (default : /var/ax25/axp2)
  -f, --frequency=FREQUENCY_HZ   Frequency in Hz (default: 433600000)
  -F, --fec                  Activate FEC (default off)
  -H, --long-help            Print a long help and exit
  -l, --packet-delay=DELAY_UNITS   Delay between successive radio blocks when
                             transmitting a larger block. In 2-FSK byte
                             duration units. (default 30)
  -m, --modulation-index=MODULATION_INDEX
                             Modulation index (default 0.5)
  -M, --modulation=MODULATION_SCHEME
                             Radio modulation scheme, See long help (-H)
                             option
  -n, --repetition=REPETITION   Repetiton factor wherever appropriate, see long
                             Help (-H) option (default : 1 single)
  -P, --packet-length=PACKET_LENGTH
                             Packet length (fixed) or maximum packet length
                             (variable) (default: 250)
  -R, --rate=DATA_RATE_INDEX Data rate index, See long help (-H) option
  -s, --radio-status         Print radio status and exit
  -t, --test-mode=TEST_SCHEME   Test scheme, See long help (-H) option fpr
                             details (default : 0 no test)
      --tnc-keydown-delay=KEYDOWN_DELAY_US
                             FUTUR USE: TNC keydown delay in microseconds
                             (default: 0 inactive)
      --tnc-keyup-delay=KEYUP_DELAY_US
                             TNC keyup delay in microseconds (default: 4ms).
                             In KISS mode it can be changed live via
                             kissparms.
      --tnc-radio-window=RX_WINDOW_US
                             TNC time window in microseconds for concatenating
                             radio frames. 0: no concatenation (default: 0))
      --tnc-serial-window=TX_WINDOW_US
                             TNC time window in microseconds for concatenating
                             serial frames. 0: no concatenation (default:
                             40ms))
      --tnc-switchover-delay=SWITCHOVER_DELAY_US
                             FUTUR USE: TNC switchover delay in microseconds
                             (default: 0 inactive)
  -T, --real-time            Engage so called "real time" scheduling (defalut
                             0: no)
  -v, --verbose=VERBOSITY_LEVEL   Verbosiity level: 0 quiet else verbose level
                             (default : quiet)
  -V, --variable-length      Variable packet length. Given packet length
                             becomes maximum length (default off)
  -w, --rate-skew=RATE_MULTIPLIER
                             Data rate skew multiplier. (default 1.0 = no
                             skew)
  -W, --whitening            Activate whitening (default off)
  -y, --test-phrase=TEST_PHRASE   Set a test phrase to be used in test (default
                             : "Hello, World!")
  -?, --help                 Give this help list
      --usage                Give a short usage message
      --version              Print program version
</code></pre>

Note: variable length blocks are not implemented yet.

## Detailed options
### Verbosity level (-v)
It ranges from 0 to 4:
  - 0: nothing at all
  - 1: Errors and some warnings and one line summary for each block sent or received
  - 2: Adds details on received blocks like RSSI and LQI
  - 3: Adds details on interrupt calls
  - 4: Adds full hex dump of sent and received blocks

Be aware that printing out to console takes time and might cause problems when transfer speeds and interactivity increase.

### Radio interface speeds (-R)
 <pre><code>
Value: Rate (Baud):
 0     50 (experimental)
 1     110 (experimental)
 2     300 (experimental)
 3     600
 4     1200
 5     2400
 6     4800
 7     9600
 8     14400
 9     19200
10     28800
11     38400
12     57600
13     76800
14     115200
15     250000
16     500000 (300000 for 4-FSK)
</code></pre>

### Modulations (-M)
 <pre><code>
Value: Scheme:
0      OOK
1      2-FSK
2      4-FSK
3      MSK
4      GFSK
</code></pre>

Note: MSK does not seem to work too well at least with the default radio options.

### Test routines (-t)
 <pre><code>
Value: Scheme:
0      No test (KISS virtual TNC)
1      Simple Tx with polling. Packet smaller than 64 bytes
2      Simple Tx with packet interrupt handling. Packet up to 255 bytes
3      Simple Rx with polling. Packet smaller than 64 bytes
4      Simple Rx with packet interrupt handling. Packet up to 255 bytes
5      Simple echo test starting with Tx
6      Simple echo test starting with Rx
</code></pre>

# AX.25/KISS operation
## Set up the AX.25/KISS environment
### Kernel modules
You will need to activate the proper options in the `make menuconfig` of your kernel compilation in order to get the `ax25` and `mkiss` modules. It comes by default in most pre-compiled kernels.

Load the modules with `modprobe` command:
  - `sudo modprobe ax25`
  - `sudo modprobe mkiss`

Alternatively you can specify these modules to be loaded at boot time by adding their names in the `/etc/modules` file

### Install AX.25 and KISS software
  - `sudo apt-get install ax25-apps ax25-node ax25-tools libax25`

### Create your AX.25 interfaces configuration
In `/etc/ax25/axports` you have to add a line with:
  - `<interface name> <callsign and suffix> <speed> <paclen> <window size> <comment>`
  - *interface name* is any name you will refer this interface to later
  - *callsign and suffix* is your callsign and a suffix from 0 to 15. Ex: `F4EXB-14` and is the interface hardware address for AX.25 just like the MAC address is the hardware address for Ethrnet.
  - *speed* is the speed in Baud. This has not been found really effective. The speed will be determined by the settings of the CC1101 itself and the TCP/IP flow will adapt to the actual speed.
  - *paclen* this is the MTU of the network interface (ax0). Effectively this sets the limit on the size of each individual KISS frame although several frames can be concatenated. The value 224 along with a fixed radio block size (-P parameter) of 252 has been found satisfactory in most conditions.  
  - *window size* is a number from 1 to 7 and is the maximum number of packets before an acknowledgement is required. This doesn't really work with KISS. KISS determines how many packets can be combined together in concatenated KISS frames that are sent as a single block. On the other end of the transmission the ACK can only be returned after the whole block has been received.
  - *comment* is any descriptive comment

Example:
 <pre><code>
 # /etc/ax25/axports
 #
 # The format of this file is:
 #
 # name callsign speed paclen window description
 #
 radio0  F4EXB-14           9600  224     1       Hamnet CC1101
 radio1  F4EXB-15           9600  224     1       Hamnet CC1101
 #1      OH2BNS-1           1200  255     2       144.675 MHz (1200  bps)
 #2      OH2BNS-9          38400  255     7       TNOS/Linux  (38400 bps)
</code></pre>

### Create a virtual serial link
 - `socat d -d pty,link=/var/ax25/axp1,raw,echo=0 pty,link=/var/ax25/axp2,raw,echo=0 &`

Note the `&` at the end that allows the command to run in background.

This creates two serial devices at the end of a virtual serial cable. 
They are accessible via the symlinks specified in the command:
  - /var/ax25/axp1
  - /var/ax25/axp2

AX.25/KISS engine will be attached to the `axp1` end and the program to `axp2`.

### Create the network device using kissattach
  - `sudo kissattach /var/ax25/axp1 radio0 10.0.1.7`
  - `sudo ifconfig ax0 netmask 255.255.255.0`

This will create the `ax0` network device as shown by the `/sbin/ifconfig` command:
 <pre><code>
ax0       Link encap:AMPR AX.25  HWaddr F4EXB-15  
          inet addr:10.0.1.7  Bcast:10.0.1.255  Mask:255.255.255.0
          UP BROADCAST RUNNING  MTU:224  Metric:1
          RX packets:3033 errors:24 dropped:0 overruns:0 frame:0
          TX packets:3427 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:10 
          RX bytes:483956 (472.6 KiB)  TX bytes:446797 (436.3 KiB)
</code></pre>

### Scripts that will run these commands
In the `scripts` directory you will find:
  - `kissdown.sh`: kills all processes and removes the `ax0` network interface from the system
  - `kissup.sh <IP> <Netmask>`: brings up the `ax0` network interface with IP address <IP> and net mask <Netmask>

Examples:
  - `./kissdown.sh`
  - `./kissup.sh 10.0.1.3 255.255.255.0`

## Run the program
This example will set the CC1101 at 9600 Baud with GFSK modulation. We raise the priority of the process (lower the priority number down to 0) with the `nice` command:

  - `sudo nice -n -20 ./picc1101 -v1 -B 9600 -P 252 -R7 -M4 -W -l15`

Other options are:
  - verbosity level (-v) of 1 will only display basic execution messages, errors and warnings
  - radio block size (-P) is fixed at 252 bytes
  - data whitening is in use (-W)
  - inter-block pause when sending multiple blocks (see next) is set for a 15 bytes transmission time approximately (-l) 

Note that you have to be super user to execute the program.

## Relay the KST chat
As a sidenote this is the way you can relay the KST chat (that is port 23000 of a specific server) through this radio link. 

On one end that has a connection to the Internet (say 10.0.1.3) do the port forwarding:
  - `sudo /sbin/iptables -t nat -A PREROUTING -p tcp -i ax0 --dport 23000 -j DNAT --to-destination 188.165.198.144:23000`
  - `sudo /sbin/iptables -t nat -A POSTROUTING -p tcp --dport 23000 -j MASQUERADE`

On the other end (say 10.0.1.7) use a telnet chat client such as the [modified colrdx for KST](https://github.com/f4exb/colrdx) and connect using the one end's IP address and port 23000:
  - `colrdx -c (callsign) -k 10.0.1.3 23000`

# Details of the design
## Multiple block handling
The CC1101 can transmit blocks up to 255 bytes. There is a so called "Infinite block" option but we don't want to use it here. In order to transmit larger blocks which is necessary for concatenated KISS frames or effective MTUs larger than 255 bytes we simply use a block countdown scheme. Each block has a header of two bytes:
  - Byte 0 is the length of the actual block of data inside the fixed size block 
  - Byte 1 is a block countdown counter that is decremented at each successive block belonging to the same larger block to transmit. Single blocks are simply transmitted with a countdown of 0 and so is the last block of a multiple block group.

When transmitting a sequence of blocks the first block is set to the result of the Euclidean division of the greater block size by the radio block size and it is decremented at each successive radio block to send until it reaches zero for the last block.

At the reception end the radio block countdown is checked and if it is not zero it will expect a next block with a countdown counter decremented by one until it receives a block with a countdown of zero.

This allows the transmission of greater blocks of up to 2^16 = 64k = 65536 bytes.

If any block is corrupted (bad CRC) or if its countdown counter is out of sequence then the whole greater block is discarded. This effectively puts a limit on the acceptable fragmentation depending on the quality of the link.

## Mitigate AX.25/KISS spurious packet retransmissions
In the latest versions an effort has been made to try to mitigate unnecessary packet retransmissions. These are generally caused by fragmenting packet chains too early. In return the ACK from the other end is received too early and synchronization is broken. Because of its robust handshake mechanism TCP/IP eventually recovers but some time is wasted.

To mitigate this effect when a packet is received on the serial link if another packet is received before some delay expires it is concatenated to the previous packet(s). The packets are sent over the air after this delay or if a radio packet has been received. This delay is called the TNC serial window.

The same mechanism exists on the radio side to possibly concatenate radio packets before they are sent on the serial line. The corresponding delay is called the TNC radio window.

These delays can be entered on the command line with the following long options with arguments in microseconds. The defaults have proved satisfactory on a 9600 Baud 2-FSK with 252 byte packets transmission. You may want to play with them or tweak them for different transmission characteristics:
  - `--tnc-serial-window`: defaults to 40ms. 
  - `--tnc-radio-window`: defaults to 0 that is no delay. Once the packet is received it will be immediately transfered to the serial link. At 9600 Baud 2-FSK with 250 byte packets the transmission time is already 208ms.
  
