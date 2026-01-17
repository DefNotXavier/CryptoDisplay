#!/bin/bash
   export DISPLAY=:0

   while true; do
      # Check if the config file on the USB exists
      if [ -f /media/pi/CRYPTOS/config.json ]; then
         # Check if the app is ALREADY running (so we don't open it 100 times)
         if ! pgrep -x "RASPBERRYPROJECT" > /dev/null; then
               cd /home/pi/Desktop/RaspberryProject/build
               ./RASPBERRYPROJECT
         fi
      fi
      sleep 2 # Check every 2 seconds
   done