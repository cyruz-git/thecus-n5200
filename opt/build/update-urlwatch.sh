#!/bin/bash
#
# Clone urlwatch git repository and create a minimal deb.
# Require: python-concurrent.futures python-keyring

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
for i in python dpkg-deb; do
        type $i >/dev/null 2>&1 || { echo "Error: $i not installed. Aborting."; exit 1; }
done

rm -rf urlwatch
git clone https://github.com/thp/urlwatch.git
cd urlwatch

python setup.py bdist
cd dist

mkdir -p debian/DEBIAN
tar xvzf urlwatch-*.tar.gz -C debian
VER=$(ls urlwatch-* | grep -Po '(?<=urlwatch-)[1-9]*\.[1-9]*')
cat > debian/DEBIAN/control <<EOF
Package: urlwatch
Version: $VER
Section: net
Priority: optional
Architecture: all
Depends: python, python (>= 3.2) | python-concurrent.futures, python-keyring
Maintainer: cyruz <focabresm@gmail.com>
Description: A tool for monitoring webpages for updates
 This is a simple URL watcher, designed to send you diffs of webpages as they change.
 Ideal for watching web pages of university courses, so you always know when lecture
 dates have changed or new tasks are online :)
EOF

dpkg-deb --build debian
echo Moving debian.deb to urlwatch-$VER.deb
mv debian.deb urlwatch-$VER.deb
echo urlwatch-$VER.deb present in urlwatch/dist

