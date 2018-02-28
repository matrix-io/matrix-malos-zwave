#!/bin/bash

case $1 in
  "copy" )
    sudo cp -avr /usr/share/matrixlabs/matrixio-devices/blob/system_creator.bit /usr/share/matrixlabs/matrixio-devices/blob/system_creator_zigbee.bit
    sudo cp -avr ./system_zwave.bit /usr/share/matrixlabs/matrixio-devices/blob/system_creator.bit
    echo "MATRIXIO Creator init FPGA firmware has been replaced to support Zwave in ttyS0"
    ;;
  "remove")
    sudo cp -avr /usr/share/matrixlabs/matrixio-devices/blob/system_creator_zigbee.bit /usr/share/matrixlabs/matrixio-devices/blob/system_creator.bit
    echo "Set Defult MATRIXIO Creator init FPGA firmware with Zigbee in ttyS0"
    ;;
  *)
  echo "Use: "
  echo " ./fpga_program.bash copy " 
  echo "       Replace FPGA firmware with Zwave port in ttyS0"
  echo "./fpga_program.bash remove" 
  echo "       Set default FPGA firmware with ZigBee port in ttyS0" 
esac

exit 1

