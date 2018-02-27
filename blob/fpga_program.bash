#!/bin/bash

function reset_creator(){
  echo 18 > /sys/class/gpio/export 2>/dev/null
  echo out > /sys/class/gpio/gpio18/direction
  echo 1 > /sys/class/gpio/gpio18/value
  echo 0 > /sys/class/gpio/gpio18/value
  echo 1 > /sys/class/gpio/gpio18/value
}

function try_program_creator() {
  reset_creator
  sleep 0.1
  xc3sprog -c matrix_creator system_zwave.bit -p 1 > /dev/null 2> /dev/null
}

function program_creator(){
count=0
while [  $count -lt 5 ]; do
  try_program_creator
  if [ $? -eq 0 ];then
        echo "**** MATRIX Creator FPGA has been programmed!"
	./fpga_info
        exit 0
   fi
  let count=count+1
done
}

echo "**** Could not program FPGA"
exit 1

