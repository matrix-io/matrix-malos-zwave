#!/bin/bash

case $1 in
  "copy" )
    cp -avr /usr/share/matrixlabs/matrixio-devices/blob/system_creator.bit /usr/share/matrixlabs/matrixio-devices/blob/system_creator_zigbee.bit
    cp -avr ./system_zwave.bit /usr/share/matrixlabs/matrixio-devices/blob/system_creator.bit
    echo ""
    echo " *** MATRIXIO Creator init FPGA firmware has been replaced to support Zwave in ttyS0 *** "
    ;;
  "remove")
    mv /usr/share/matrixlabs/matrixio-devices/blob/system_creator_zigbee.bit /usr/share/matrixlabs/matrixio-devices/blob/system_creator.bit
    echo ""
    echo " *** Set Defult MATRIXIO Creator init FPGA firmware with Zigbee in ttyS0 *** "
    ;;
  *)
  echo "Use: "
  echo " ./zwave_setup.bash copy " 
  echo "       Replace FPGA firmware with Zwave port in ttyS0"
  echo "./zwave_setup.bash remove" 
  echo "       Set default FPGA firmware with ZigBee port in ttyS0" 
esac

" *** Reboot your system!"

exit 1

