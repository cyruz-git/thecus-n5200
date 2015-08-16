#!/bin/bash
#
# Trivially clone bjoern repository and compile its source.
# Require: python-dev libev-dev git

[[ $(id -u) -ne 0 ]] && { echo "Please run as root."; exit 1; }
type git >/dev/null 2>&1 || { echo "Error: git not installed. Aborting."; exit 1; }

rm -rf bjoern
git clone --recursive https://github.com/jonashaag/bjoern.git
cd bjoern

python setup.py install
