#!/bin/bash

DEVICES=$(avahi-browse _z-wave._udp -r -t -p | grep Controller| cut -d ";" -f8)


/usr/bin/malos_zwave  -server "${DEVICE}" \
                      -xml /etc/zwave/ZWave_custom_cmd_classes.xml
