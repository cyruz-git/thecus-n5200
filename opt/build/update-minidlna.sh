#!/bin/bash
# minidlna updater script.

PKGNAME="minidlna"
REQUIREDPKG="libavformat-dev libavutil-dev libexif-dev libflac-dev libid3tag0-dev libjpeg8-dev libogg-dev libsqlite3-dev libvorbis-dev autopoint debhelper dh-autoreconf dpkg-dev"

# Import common functions.
. "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/common/common-functions.sh" || exit 1

# Parse commandline arguments.
parse_arguments $@

# Prerequisites.
check_root
check_packages $REQUIREDPKG
[[ -n "$MISSINGPKG" ]] && install_packages $MISSINGPKG

# Version check and sources update.
deb_check_version $PKGNAME
deb_update_sources $PKGNAME
cd $PKGNAME*

echo "Build deb package."
dpkg-buildpackage -us -uc

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
