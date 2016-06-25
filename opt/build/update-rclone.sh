#!/bin/bash
# rclone updater script.

PKGNAME="rclone"
REQUIREDPKG="wget unzip dpkg-dev"

# Import common functions.
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common/common-functions.sh" || exit 1

# Parse commandline arguments.
parse_arguments $@

# Prerequisites.
check_root
check_packages $REQUIREDPKG
[[ -n "$MISSINGPKG" ]] && install_packages $MISSINGPKG

# Version check.
VER=$(wget -qO- http://rclone.org/downloads/ | grep -Po '(?<=Rclone Download v)[\d\.]*')
deb_check_version $VER

echo "Download $PKGNAME archive."
rm -rf $PKGNAME && mkdir $PKGNAME && cd $PKGNAME
wget http://downloads.rclone.org/rclone-current-linux-386.zip

echo -en "Extracting $PKGNAME archive... "
unzip rclone-current-linux-386.zip > /dev/null 2>&1 && rm -f rclone-current-linux-386.zip
echo -en "[OK]\n"

echo -en "Setting up debian packaging files... "
DIR=$(ls)
mkdir -p debian/DEBIAN 
cat > debian/DEBIAN/control << EOF
Package: $PKGNAME
Version: $VER
Section: net
Priority: optional
Architecture: i386
Depends:
Maintainer: cyruz <focabresm@gmail.com>
Description: Commandline program to sync files and directories to and from many cloud hosting services.
EOF
mkdir -p debian/usr/local/bin && cp "$DIR/rclone" debian/usr/local/bin
mkdir -p debian/usr/local/share/man/man1 && cp "$DIR/rclone.1" debian/usr/local/share/man/man1
echo -en "[OK]\n"

echo "Build deb package."
dpkg-deb --build debian ./

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
