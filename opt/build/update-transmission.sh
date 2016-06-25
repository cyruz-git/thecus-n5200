#!/bin/bash
# transmission updater script.

PKGNAME="transmission"
REQUIREDPKG="ca-certificates libcurl4-openssl-dev libssl-dev libevent-dev intltool make"

# Import common functions.
BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common"
if [[ ! -f $BASEDIR/common-functions.sh  || ! -f $BASEDIR/$PKGNAME-files.tar.gz ]]; then
    echo "Common functions or files not available. Aborting."
    exit 1
fi
. $BASEDIR/common-functions.sh

# Parse commandline arguments.
parse_arguments $@

# Prerequisites.
check_root
check_packages $REQUIREDPKG
[[ -n "$MISSINGPKG" ]] && install_packages $MISSINGPKG

# Version check and sources update.
deb_check_version $PKGNAME
deb_update_sources $PKGNAME --download-only

echo -en "Creating debian packaging tree... "
mkdir debian && tar xzf ../common/$PKGNAME*-files.tar.gz -C debian
DEBTREE=$PWD"/debian"
echo -en "[OK]\n"

echo -en "Extracting version number... "
VER=$(apt-cache showsrc transmission | grep ^Version: | cut -d' ' -f2)
sed -i "s/Version:/Version: ${VER}/" debian/DEBIAN/control 
echo -en "[OK]\n"

echo -en "Extracting $PKGNAME archive... "
tar xf $PKGNAME*.orig.tar.xz && cd $PKGNAME*
echo -en "[OK]\n"

echo "Configure sources."
CFLAGS="-O2 -march=pentium-m -pipe -fomit-frame-pointer" ./configure

echo "Compile sources and install binaries in the debian packaging tree."
make && make install DESTDIR="$DEBTREE"

echo "Build deb package."
cd .. && dpkg-deb --build debian .

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
