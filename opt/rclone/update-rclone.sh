#!/bin/bash
#
# Update rclone

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
for i in wget unzip
do
	type $i >/dev/null 2>&1 || { echo "Error: $i not installed. Aborting."; exit 1; }
done

rm -f rclone* README*
wget http://downloads.rclone.org/rclone-current-linux-386.zip
unzip rclone-current-linux-386.zip
rm -f rclone-current-linux-386.zip
mv rclone-*/* .
rm -rf rclone-*/
