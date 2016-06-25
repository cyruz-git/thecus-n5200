#!/bin/bash
# urlwatch updater script.

PKGNAME="urlwatch"
REQUIREDPKG="python-concurrent.futures python-keyring"

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
