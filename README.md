# Pre-Requisites
```
# Add repo and key
curl https://apt.matrix.one/doc/apt-key.gpg | sudo apt-key add -
echo "deb https://apt.matrix.one/raspbian $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/matrixlabs.list

# Update packages and install
sudo apt-get update
sudo apt-get upgrade

# Install dependencies
apt-get install --yes libmatrixio-malos, matrixio-libzwaveip, matrixio-zipgateway

```


# Install from source

## Pre-Requisites
It does have some package dependencies, so please make sure to install the pre-requisites.

```
sudo apt-get install cmake g++ git libmatrixio-protos-dev libmatrixio-malos-dev libreadline-dev matrixio-libzwaveip-dev libxml2-dev libbsd-dev libncurses5-dev  libavahi-client-dev avahi-utils libreadline-dev libgflags-dev
```

* Using **Raspbian Jessie** install:

```
sudo apt-get install --yes libssl-dev
```

* Using **Raspbian Stretch** install:

```
sudo apt-get install --yes libssl1.0-dev
```


To start working with **MATRIX Zwave Malos** directly, you'll need to run the following steps: 

```
git clone https://github.com/matrix-io/matrix-malos-zwave/
cd matrix-malos-zwave && mkdir build && cd build
cmake ..
-- The C compiler identification is GNU 6.3.0
-- The CXX compiler identification is GNU 6.3.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- * * * A D M O B I L I Z E * * *
-- Admobilize: Please treat warnings as errors. Use: -DADM_FATAL_WARNINGS=ON
-- * * * * * * * * * * * * * * * *
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Looking for pthread_create
-- Looking for pthread_create - not found
-- Looking for pthread_create in pthreads
-- Looking for pthread_create in pthreads - not found
-- Looking for pthread_create in pthread
-- Looking for pthread_create in pthread - found
-- Found Threads: TRUE  
-- Found Protobuf: /usr/lib/arm-linux-gnueabihf/libprotobuf.so;-lpthread (found suitable version "3.4.0", minimum required is "3") 
-- Found OpenSSL: /usr/lib/arm-linux-gnueabihf/libssl.so;/usr/lib/arm-linux-gnueabihf/libcrypto.so (found version "1.0.2l") 
-- Found LibXml2: /usr/lib/arm-linux-gnueabihf/libxml2.so (found version "2.9.4") 
-- ZMQ found => /usr/lib/arm-linux-gnueabihf/libzmq.so
-- MATRIX LIB found => /usr/lib/libmatrix_malos.a
-- MATRIX ZMQ LIB found => /usr/lib/libmatrix_malos_zmq.a
-- ZWAVEIP found => /usr/lib/libzwaveip.a
-- XML2 found =>/usr/lib/arm-linux-gnueabihf/libxml2.so
-- gflags found =>/usr/lib/arm-linux-gnueabihf/libgflags.so
-- MATRIX_PROTOS_LIB LIB found => /usr/lib/libmatrixio_protos.a
-- avahi-common found =>/usr/lib/arm-linux-gnueabihf/libavahi-common.so
-- avahi-client found =>/usr/lib/arm-linux-gnueabihf/libavahi-client.so
-- Enabling MDNS...
--   avahi-common found =>/usr/lib/arm-linux-gnueabihf/libavahi-common.so
--   avahi-client found =>/usr/lib/arm-linux-gnueabihf/libavahi-client.so
-- Configuring done
-- Generating done
-- Build files have been written to: /home/pi/github/matrix-malos-zwave/build
make
Scanning dependencies of target malos_zwave
[ 33%] Building CXX object src/CMakeFiles/malos_zwave.dir/malos_zwave.cpp.o
[ 66%] Building CXX object src/CMakeFiles/malos_zwave.dir/driver_zwave.cpp.o
[100%] Linking CXX executable malos_zwave
[100%] Built target malos_zwave
```


# MALOS Zwave

Hardware abstraction layer for MATRIX Creator usable via 0MQ.
[Protocol buffers](https://developers.google.com/protocol-buffers/docs/proto3) are used for data exchange.

You can also use MALOS to query sensors of the [MATRIX Creator](https://creator.matrix.one) and to control the MATRIX Creator from any language that supports protocol buffers (version 3.X) and 0MQ,
Connections to MALOS can be made both from localhost (127.0.0.1) and from remote computers that are in the same network.

### Install
```
sudo apt-get install matrixio-malos-zwave
sudo reboot
```

**Note:** At this point, on next start, `matrixio-malos-zwave` will be running as a service.

``` 
sudo systemctl status matrixio-malos-zwave
```

### Upgrade
```
sudo apt-get update && sudo apt-get upgrade
sudo reboot
```

### Starting manually
```
# MALOS runs as a service, but to stop it run:
sudo pkill -9 malos-zwave

# to run manually, just type `malos`
malos-zwave
```

### Protocol
[UPDATE]

First of all, you need to know that there are some steps to follow to work:

1. Start the Z/IP Gateway
2. Find Z-Wave services in the network
3. Start the Referecence Z/IP Client
4. Include the node (Device)
5. Control the node (Device)

But these steps are following the Z/IP Gateway from Sigma Design, which we use it as a design reference.  Our middleware communicates with the gateway using [libzwaveip](https://github.com/matrix-io/libzwaveip) and it is based on the public examples in that repository. The libzwaveip is a library making it easier to control Z-Wave devices from an IP network via a Z/IP Gateway. Also we use SERIAL API (SDK 6.71). We run the Z/IP Gateway on the Raspberry Pi and communicate with the gateway using the libzwaveip.
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
1. First you need to run the Z/IP Gateway whom is already installed with `matrixio-zipgateway`
2. Run the `malos_zwave`
3. Find Z-Wave clients in the network - you can find it using the `LIST` operation
4. Just include a node (device) with `ADDNODE` operation
5. SEND commands to your device with `SEND` operation

You can also remove a node with the `REMOVENODE` operation. All operations are inside the `ZWaveMsg` object. First you need to create a new instance of `matrixMalosBuilder.ZWaveMsg`. And the set the operation you want to use with the `set_operation()` function and then send it via socket. It should be more or less like this:

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

### NodeJS Dependency

For instance (in the Raspberry):

```
# Install npm (doesn't really matter what version, apt-get node is v0.10...)
sudo apt-get install npm

# n is a node version manager
sudo npm install -g n

# node 6.5 is the latest target node version, also installs new npm
n 6.5

# check version
node -v
```