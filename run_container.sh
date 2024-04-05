#!/bin/sh

[ -z "$2" ] && echo "Usage: $0 <PRINTER_ADDRESS> <PRINTER_ACCESS_CODE>" && exit 1

PRINTER_ADDRESS=$1
PRINTER_ACCESS_CODE=$2

podman run -d --name bambu_p1_streamer -p 1984:1984 -e PRINTER_ADDRESS=$PRINTER_ADDRESS -e PRINTER_ACCESS_CODE=$PRINTER_ACCESS_CODE bambu_p1_streamer
