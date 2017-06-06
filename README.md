# Dependencies

## Creator
```
echo "deb http://packages.matrix.one/matrix-creator/ ./" | sudo tee --append /etc/apt/sources.list;
sudo apt-get update;
sudo apt-get upgrade;
sudo apt-get install libzmq3-dev xc3sprog matrix-creator-openocd wiringpi cmake g++ git matrix-creator-init matrix-creator-malos
```

## libzwaveip
```
sudo apt-get install libssl-dev libxml2-dev libbsd-dev libncurses5-dev  libavahi-client-dev avahi-utils
```

```
git clone  git@github.com:matrix-io/libzwaveip.git
 
cd libzwaveip
mkdir build
cd build
cmake ..
make && sudo make install
```

## matrix-malos-zwave
```
sudo apt install libreadline-dev libgflags-dev libzwaveip-dev
```

```
git clone git@github.com:matrix-io/matrix-malos-zwave.git
 
cd matrix-malos-zwave
 
git submodule init && git submodule update

mkdir build
cd build
cmake ..
make 
```
