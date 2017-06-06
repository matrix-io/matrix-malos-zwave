#!/bin/bash

DEVICES=$(avahi-browse _z-wave._udp -r -t | grep address| cut -d "[" -f2 | cut -d "]" -f1)

for device in $DEVICES
do
  /usr/bin/malos_zwave  -server ${device} \
                        -xml /etc/zwave/ZWave_custom_cmd_classes.xml

done

