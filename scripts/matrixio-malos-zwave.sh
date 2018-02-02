#!/bin/bash

DEVICES=$(avahi-browse _z-wave._udp -t | grep Controller| cut -d " " -f 4-6)

for device in $DEVICES
do
  /usr/bin/malos_zwave  -server "${device}" \
                        -xml /etc/zwave/ZWave_custom_cmd_classes.xml

done

  
