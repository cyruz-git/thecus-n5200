#!/bin/bash
# snapraid updater script.

PKGNAME="snapraid"
REQUIREDPKG="make"

# Import common functions.
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common/common-functions.sh" || exit 1

# Parse commandline arguments.
parse_arguments $@

# Prerequisites.
check_root
check_packages $REQUIREDPKG
[[ -n "$MISSINGPKG" ]] && install_packages $MISSINGPKG

echo -en "Get latest snapraid version url... "
URL=$(wget -qO- http://www.snapraid.it/download | grep -Po 'https[^"]+snapraid[\d\-\.]*\.tar\.gz')
echo -en "[OK]\n"

# Version check.
[[ $URL =~ ([0-9\.]*).tar.gz ]] && VER=${BASH_REMATCH[1]} || { echo "ERROR: wrong regexp."; exit 1; }
deb_check_version $PKGNAME $VER

echo "Download $PKGNAME archive."
rm -rf $PKGNAME && mkdir $PKGNAME && cd $PKGNAME && DIR=$(pwd)
wget $URL

echo -en "Extracting $PKGNAME archive... "
[[ $URL =~ \/([^\/]*\.tar\.gz) ]] && tar xzf ${BASH_REMATCH[1]} || { echo "ERROR: wrong regexp."; exit 1; }
echo -en "[OK]\n"

echo -en "Setting up debian packaging files... "
mkdir -p debian/DEBIAN && DEBTREE=$PWD"/debian"
cat > debian/DEBIAN/control << EOF
Package: $PKGNAME
Version: $VER
Section: admin
Priority: optional
Architecture: i386
Depends:
Homepage: http://www.snapraid.it/
Maintainer: cyruz <focabresm@gmail.com>
Description: Backup program for disk arrays.
EOF
echo -en "[OK]\n"

echo "Configure sources."
cd $PKGNAME* && ./configure

echo "Compile sources and install binaries in the debian packaging tree."
make && make install DESTDIR="$DEBTREE"

echo "Build deb package."
cd .. && dpkg-deb --build debian .

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
