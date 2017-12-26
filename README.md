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