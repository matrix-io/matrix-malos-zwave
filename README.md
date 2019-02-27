# MALOS (MATRIX Core) Zwave

Zwave abstraction layer for MATRIX Creator via 0MQ.
[Protocol buffers](https://developers.google.com/protocol-buffers/docs/proto3) are used for data exchange.

This library allows listing, adding, removing, and controlling Zwave nodes.

You can also use MALOS ([MATRIX Core](https://matrix-io.github.io/matrix-documentation/matrix-core/overview/)) to query sensors of the [MATRIX Creator](https://creator.matrix.one) and to control the MATRIX Creator from any language that supports protocol buffers (version 3.X) and 0MQ,
Connections to MALOS can be made both from localhost (127.0.0.1) and from remote computers that are on the same network.


### Zwave Initial Setup

Ensure you have a Raspberry Pi, attached with a MATRIX Creator, that's flashed with [**Raspbian Stretch**](https://www.raspberrypi.org/downloads/raspbian/).

```bash
# Add MATRIX repo and key
curl https://apt.matrix.one/doc/apt-key.gpg | sudo apt-key add -
echo "deb https://apt.matrix.one/raspbian $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/matrixlabs.list

# Update packages and install
sudo apt-get update
sudo apt-get upgrade

# Install MATRIX Creator Init
sudo apt-get install matrixio-creator-init matrixio-kernel-modules

# Reboot your device
sudo reboot

# Install Zwave Utils 
sudo apt-get install matrixio-zwave-utils
```

### Zwave Utils Setup

Run `zwave_conf` in order to set up the ZM5202 Zwave module in the MATRIX Creator. 

```bash
$ zwave_conf 
ID.conf: line 1: INFO:: command not found
*** DE88C MATRIX Creator US has been detected 
INFO: [/dev/matrixio_regmap] was opened
#ZM5202 Response: 53 : AA
#Serie 500 chip found
#Chip Ready
#Chip Ready
#Chip Ready
#NVR check process OK 
#NVR check process OK 
NVR_Status=1
#Zwave NVR has been programmed !!! 
INFO: [/dev/matrixio_regmap] was opened
#ZM5202 Response: 53 : AA
Status: 0
Chip Ready
....0
....1
....2
....3
....4
....5
....6
....7
....8
....15
....16
....17
....18
....19
....20
....21
....22
....23
....24
....25
....26
....27
....28
....29
....30
....31
....32
....33
....34
....35
....36
....37
....48
....49
....50
....51
....52
....53
....54
....55
....63
*** Zwave Init Set Up Complete. Reboot your device
```

Then reboot your device. **This process should be run just one time**. The ZM5202 will keep this configuration.

Once you ssh back into your Raspberry Pi, you should see two new files, ID.conf and NVR_file. If not, wait a couple of seconds for them to appear before you move on to the next steps.

### Install MATRIX MALOS Zwave
```bash
sudo apt-get install matrixio-malos-zwave
```
Near the end of this installation process, you will be prompted to configure **zipgateway** for the Zwave controller. The configuration settings should be as shown in the following section.

If you are not prompted to configure **zipgateway**, run the above command to install **matrixio-malos-zwave** again.

#### Configuration

The matrixio-kernel-modules enable a new serial port called `ttyMATRIX0`. This port is the communication channel to the ZM5202. The **zipgateway** is shown below.

* The serial port where z-wave controller is connected: **/dev/ttyMATRIX0** (You will have to rename the port manually) [mandatory]
* The IPv6 address of the Z/IP Gateway: **fd00:aaaa::3** [optional]
* IPv6 prefix of the Z-Wave network: **fd00:bbbb::1** [optional]
* Enable wireless configuration of Z/IP Gateway: **wired** [optional]
* Wired network interface where the ZIP Client will be connected to: **eth0** [optional]

You will then be prompted to reboot your device. Select "yes," then wait for your Pi to shutdown (do not force reboot at this point, background processes have to finish running before shutdown is initiated). Once rebooted, ssh back into your Raspberry Pi.

You could check if zipgateway is runing with `more /tmp/zipgateway.log` as shown below:
```bash
$ more /tmp/zipgateway.log

17170 Opening config file /usr/local/etc/zipgateway.cfg
Starting Contiki
Opening eeprom file /usr/local/var/lib/zipgateway/eeprom.dat
Lan device tap0
LAN HW addr B6:0C:53:20:46:76
17185 Starting zipgateway ver2_61 build ver2_61
17185 Resetting ZIP Gateway
17185 Using serial port /dev/ttyMATRIX0
 SerialAPI: Serial API version     : 5.34
17264 500 series chip version 0
17267 I'am SUC
17411 Key  class  80 
F83C9C7888E766D743EA03DB7D97656B
17422 Key  class  1 
95F2E42BDE04EDEA8A3FAC9C4AB1339B
17430 Key  class  2 
0135D1F0BD3F692DAC03B98BAA009247
17437 Key  class  4 
690EB5CC6C6021418F29857142A5845F
17441 Network shceme is:17441 S2 ACCESS
17443 Resetting IMA
17443 I'm a primary or inclusion controller.
17443 Command classes updated
17446  nodeid=1 0
17446 Checking for new sessions
17446 We should send a discover
17450 Waiting for bridge
17458 Version: Z-Wave 4.61, type 7
17483 ................. the  version 0 ...............17489 NVM version is 2
17494 L2 HW addr 00:1e:32:1b:5d:f2
17494 
17494 ZIP_Router_Reset: pan_lladdr: fc:f7:ab:f7:00:01  Home ID = 0xf7abf7fc 
17494 Tunnel prefix ::
17494 
17494 Lan address fd00:aaaa::03
17494 
17494 Han address fd00:bbbb::01
17494 
17494 Gateway address fd00:aaaa::1234
17494 
17494 Unsolicited address fd00:aaaa::1234
17494 
 dynamic 
ECDH Public key is 
19990-03943-17692-46976-
21533-53149-52534-13196-
26418-19095-48486-46417-
04619-62339-27522-60221-
17628 DTLS server started
17628 mDNS server started
17628 DHCP client started
17628 Starting zip tcp client
17628 ZIP TCP Client Started.
17628 No portal host defined. Running without portal.
```
The zipgateway output may be very long. As long as the beginning of the output looks something like the above, you can hit Ctrl+C to exit the log.

#### Running as a service
At this point, `matrixio-malos-zwave` will be running as a service called `matrixio-malos-zwave.service`. You can see the status of the service using the command below.

```bash
sudo systemctl status matrixio-malos-zwave
```


# Install Dependencies to Run Zwave Command Files

### NodeJS

In your Raspberry Pi's terminal, run the following.

```bash
# Install npm (doesn't really matter what version, apt-get node is v0.10...)
sudo apt-get install npm

# install node version manager, then install version 8.6 of node.js
curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
. ~/.bashrc
nvm install 8.6

# check version
node -v
```

Download the required dependencies to clone our Zwave repo & build the Zwave files in MATRIX Core.

## Dependencies

```bash
sudo apt-get install cmake g++ git libmatrixio-protos-dev libmatrixio-malos-dev libreadline-dev matrixio-libzwaveip-dev libxml2-dev libbsd-dev libncurses5-dev  libavahi-client-dev avahi-utils libreadline-dev libgflags-dev
```
Then install the following:

```bash
sudo apt-get install --yes libssl1.0-dev
```

To start working with **MATRIX Zwave Malos** right away, run the following steps: 

```bash
git clone https://github.com/matrix-io/matrix-malos-zwave/
cd matrix-malos-zwave
git checkout si/guide-edits
mkdir build && cd build
cmake ..
make
```
To start working with MATRIX Zwave through Javascript, navigate to the following directory and install all required node dependencies from the package.json file:
```bash
cd ~/matrix-malos-zwave/src/js_test
npm install
```
This directory has 4 files to list, add, and remove Zwave nodes, as well as a simple on/off control test example.

For example, to list the Zwave components connected to your MATRIX Creator, run the following:
```bash
node list.js
```
You can write your own control algorithms by creating Javascript files built off of simpleTest.js.

If you would like to test the test_zwave_accel.js file, you will have to download MATRIX Core (previously knows as MALOS) to allow IMU data to stream.

Run the following commands in your Pi to install and run MATRIX Core:

```bash
#Install MATRIX Core package
sudo apt-get install matrixio-malos

#Reboot device so MATRIX Core service runs on next start
sudo reboot
```

### How the Protocol Works

First of all, you need to know that there are some steps to follow to work:

1. Start the Z/IP Gateway
2. Find Z-Wave services in the network
3. Start the Reference Z/IP Client
4. Include the node (Device)
5. Control the node (Device)

But these steps are following the Z/IP Gateway from Sigma Design, which we use it as a design reference.  Our middleware communicates with the gateway using [libzwaveip](https://github.com/matrix-io/libzwaveip) and it is based on the public examples in that repository. The libzwaveip is a library making it easier to control Z-Wave devices from an IP network via a Z/IP Gateway. Also, we use SERIAL API (SDK 6.71). We run the Z/IP Gateway on the Raspberry Pi and communicate with the gateway using the libzwaveip.
The Z/IP Gateway is a Z-Wave controller that does the translation between IP and Z-Wave RF. More about Z/IP Gateway can be found <a href="https://z-wave.sigmadesigns.com/wp-content/uploads/Z-IP_Gateway_br.pdf">here</a>.
Our Software Architecture follows these specs:

<img src="https://github.com/matrix-io/matrix-malos-zwave/blob/master/software_architecture.png" align="right" width="500">

### Using the Driver

We have 5 (five) operations in `ZWaveOperations` property that you can work on. They are:

+ SEND
+ ADDNODE
+ REMOVENODE
+ SETDEFAULT
+ LIST

The Z/IP Gateways 
Remember the steps mentioned earlier and now they will be like this:
1. First, you need to run the Z/IP Gateway, it is already installed with `matrixio-zipgateway`
2. Run the `malos_zwave`
3. Find Z-Wave clients in the network - you can find it using the `LIST` operation
4. Just include a node (device) with `ADDNODE` operation
5. SEND commands to your device with `SEND` operation

You can also remove a node with the `REMOVENODE` operation. All operations are inside the `ZWaveMsg` object. First, you need to create a new instance of `matrixMalosBuilder.ZWaveMsg`. And the set the operation you want to use with the `set_operation()` function and then send it via socket. It should be more or less like this:

```
...
    var zwave_cmd = new matrixMalosBuilder.ZWaveMsg;
    zwave_cmd.set_operation(matrixMalosBuilder.ZWaveMsg.ZWaveOperations.LIST);

    var config = new matrixMalosBuilder.DriverConfig;
    config.set_zwave(zwave_cmd);
    configSocket.send(config.encode().toBuffer());
...
```
-------------------------
