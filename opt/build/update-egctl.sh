#!/bin/bash
# egctl updater script.

PKGNAME="egctl"
REQUIREDPKG="git"

# Import common functions.
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common/common-functions.sh" || exit 1

# Parse commandline arguments.
parse_arguments $@

# Prerequisites.
check_root
check_packages $REQUIREDPKG
[[ -n "$MISSINGPKG" ]] && install_packages $MISSINGPKG

# Version check and sources update.
[[ ! -d $PKGNAME ]] && mkdir $PKGNAME
cd $PKGNAME
git_check_version $PKGNAME
git_update_sources https://github.com/unterwulf/egctl.git

echo -en "Setting up debian packaging files... "
mkdir -p debian/DEBIAN && DEBTREE=$PWD"/debian"
cat > debian/DEBIAN/control << EOF
Package: $PKGNAME
Version: $(date +"%Y-%m-%d")
Section: net
Priority: optional
Architecture: i386
Depends:
Homepage: https://github.com/unterwulf/egctl
Maintainer: cyruz <focabresm@gmail.com>
Description: Program to control the state of EnerGenie Programmable surge protector with LAN interface.
EOF
echo -en "[OK]\n"

echo "Compile sources."
cd egctl && make

echo "Install binaries in the debian packaging tree."
make install DESTDIR="$DEBTREE"

echo "Build deb package."
cd .. && dpkg-deb --build debian .

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
