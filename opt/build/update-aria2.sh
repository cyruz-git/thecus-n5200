#!/bin/bash
#
# Trivially download aria2 source and create a deb.
# Require: libc-ares-dev libgcrypt11-dev libsqlite3-dev libxml2-dev zlib1g-dev 
#          libssh2-1-dev nettle-dev libgnutls-dev libgmp-dev pkg-config dpkg-dev
#
# DO NOT USE `apt-get build-dep aria2` on Debian 7.0 (Wheezy) if the sources come from
# a newer system, because the Debian control file specifies newer versions for libraries.

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
type dpkg-buildpackage >/dev/null 2>&1 || { echo "Error: dpkg-dev not installed. Aborting."; exit 1; }

rm -rf aria2
mkdir aria2 && cd aria2
apt-get source aria2
cd aria2*

# We build ignoring dependencies specified by the Debian control file.
dpkg-buildpackage -d -us -uc
