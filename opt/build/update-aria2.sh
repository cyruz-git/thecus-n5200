#!/bin/bash
# aria2 updater script.

PKGNAME="aria2"
REQUIREDPKG="libc-ares-dev libgcrypt11-dev libsqlite3-dev libxml2-dev zlib1g-dev libssh2-1-dev nettle-dev libgnutls-dev libgmp-dev pkg-config dpkg-dev"

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
dpkg-buildpackage -d -us -uc # Ignore depndencies from Debian control file.

[[ -n "$INSTALLDEB" ]] && { echo "Install the deb."; dpkg -i $PKGNAME*.deb; }
