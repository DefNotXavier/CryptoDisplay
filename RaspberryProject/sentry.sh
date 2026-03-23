#!/bin/bash
export DISPLAY=:0

# If the mount point exists but nothing is actually mounted there, clean it up
if [ -d /media/pi/CRYPTOS ] && ! mountpoint -q /media/pi/CRYPTOS; then
    echo "Stale mount point found, cleaning up..."
    sudo umount -f /media/pi/CRYPTOS 2>/dev/null
    sudo rm -rf /media/pi/CRYPTOS 2>/dev/null
fi

# --- Main watcher loop ---
while true; do
    # Check if the config file on the USB exists
    if [ -f /media/pi/CRYPTOS/config.json ]; then
        # Check if the app is ALREADY running (so we don't open it 100 times)
        if ! pgrep -x "RASPBERRYPROJECT" > /dev/null; then
            cd /home/pi/Desktop/RaspberryProject/build
            ./RASPBERRYPROJECT
        fi
    fi
    sleep 2
done
