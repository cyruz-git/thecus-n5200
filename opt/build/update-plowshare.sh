#!/bin/bash
#
# Clone plowshare git repository, compile it and create a deb.
# Require: bash curl recode spidermonkey aview make checkinstall
#

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
for i in make checkinstall; do
	type $i >/dev/null 2>&1 || { echo "Error: $i not installed. Aborting."; exit 1; }
done

rm -rf plowshare
git clone https://github.com/mcrapet/plowshare.git
cd plowshare

VER=$(head -n1 CHANGELOG | grep -Po '(?<=plowshare \()[\d\.]*')
checkinstall --default --install=no --fstrans=no --pkgname=plowshare --pkgversion=$VER
