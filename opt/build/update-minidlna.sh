#!/bin/bash
#
# Trivially download minidlna source and create a deb.
# Require: libavformat-dev libavutil-dev libexif-dev libflac-dev libid3tag0-dev
#          libjpeg8-dev libogg-dev libsqlite3-dev libvorbis-dev dpkg-dev

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
type dpkg-buildpackage >/dev/null 2>&1 || { echo "Error: dpkg-dev not installed. Aborting."; exit 1; }

rm -rf minidlna
mkdir minidlna && cd minidlna
apt-get source minidlna
cd minidlna*

dpkg-buildpackage -us -uc
