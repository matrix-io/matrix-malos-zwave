#!/bin/bash

DEVICE=$(avahi-browse _z-wave._udp -t | grep Controller | grep Static | cut -d " " -f 4-6 | head -1)

/usr/bin/malos_zwave  -server "${DEVICE}" \
                      -xml /etc/zwave/ZWave_custom_cmd_classes.xml
