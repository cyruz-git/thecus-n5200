#!/bin/bash
#
# Trivially download transmission source, compile it and create a deb.
# Require: ca-certificates libcurl4-openssl-dev libssl-dev libevent-dev intltool make checkinstall
#
# DO NOT USE `apt-get build-dep transmission` on a Debian <= 7.0 (Wheezy) if using sources from a newer system.
# On Debian > 7.0, transmission depends on systemd and is patched for it, so we avoid patching when retrieving sources.

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
for i in make checkinstall; do
	type $i >/dev/null 2>&1 || { echo "Error: $i not installed. Aborting."; exit 1; }
done

rm -rf transmission
mkdir transmission && cd transmission
apt-get --download-only source transmission
tar xf transmission*.orig.tar.xz
cd transmission*

CFLAGS="-O2 -march=pentium-m -pipe -fomit-frame-pointer" ./configure
make

VER=$(apt-cache showsrc transmission | grep ^Version: | cut -d' ' -f2)
checkinstall --default --install=no --pkgname=transmission --pkgversion=$VER
mv transmission*.deb ..
