#!/bin/bash
# plowshare updater script.

PKGNAME="plowshare"
REQUIREDPKG="bash coreutils curl recode spidermonkey aview make"

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
git_update_sources https://github.com/mcrapet/plowshare.git

echo -en "Extracting version number... "
VER=$(head -n1 plowshare/CHANGELOG | grep -Po '(?<=plowshare \()[\d\.]*')
echo -en "[OK]\n"

echo -en "Setting up debian packaging files... "
mkdir -p debian/DEBIAN && DEBTREE=$PWD"/debian"
cat > debian/DEBIAN/control << EOF
Package: $PKGNAME
Version: $VER
Section: net
Priority: optional
Architecture: all
Depends: bash, coreutils, curl
Recommends: recode, spidermonkey, aview
Homepage: https://github.com/mcrapet/plowshare
Maintainer: cyruz <focabresm@gmail.com>
Description: Set of command-line tools designed for managing file-sharing websites.
EOF
echo -en "[OK]\n"

echo "Install binaries in the debian packaging tree."
cd plowshare && make install PREFIX=/usr/local DESTDIR="$DEBTREE"

echo "Build deb package."
cd .. && dpkg-deb --build debian .

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
