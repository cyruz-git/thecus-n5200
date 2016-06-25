#!/bin/bash
# bjoern updater script.

PKGNAME="bjoern"
REQUIREDPKG="dpkg-dev python-dev python-setuptools libev-dev git"

# Import common functions.
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common/common-functions.sh" || exit 1

# Parse commandline arguments.
parse_arguments $@

# Prerequisites.
check_root
check_packages $REQUIREDPKG
[[ -n "$MISSINGPKG" ]] && install_packages $MISSINGPKG

# Version check and sources update.
git_check_version $PKGNAME
git_update_sources https://github.com/jonashaag/bjoern.git

echo "Setup bjoern."
cd $PKGNAME && python setup.py bdist

echo -en "Extracting version number... "
FILE=$(ls dist/*i686*)
VER=$(ls dist/*i686* | grep -Po '(?<=bjoern-)[\d\.]*(?=\.)')
echo -en "[OK]\n"

echo -en "Setting up debian packaging files... "
mkdir -p debian/DEBIAN
tar xzf $FILE -C debian
cat > debian/DEBIAN/control << EOF
Package: $PKGNAME
Version: $VER
Section: net
Priority: optional
Architecture: all
Depends: python2.7,
         libev4
Maintainer: cyruz <focabresm@gmail.com>
Description: A screamingly fast Python WSGI server written in C.
EOF
echo -en "[OK]\n"

echo "Build deb package."
dpkg-deb --build debian ./

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
