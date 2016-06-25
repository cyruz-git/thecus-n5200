#!/bin/bash
# Common functions for updater scripts.

# Parse command line arguments and set global variables.
# Accept 2 arguments: -y and -i.
parse_arguments() {
	[[ $# > 2 ]] && { echo "ERROR: too many arguments."; exit 1; }
	for i in $@; do
		case $i in
			-y) ASSUMEYES=1 ;;
			-i) INSTALLDEB=1 ;;
			 *) ;;
		esac
	done; return 0
}

# Check if the script is run as root.
check_root() {
	echo -en "Checking root... "
	[[ $(id -u) -ne 0 ]] && { echo -en "[NOT OK]\n"; exit 1; }
	echo -en "[OK]\n"
}

# Check if the packages passed as arguments are missing.
# Fill up the MISSINGPKG variable.
# Accept a list of packages as arguments.
check_packages() {
 	[[ $# < 1 ]] && { echo "ERROR: no packages to check."; exit 1; }
	echo -en "Checking required packages... " && MISSINGPKG=""
 	for i in $@; do
 		[[ -z "$(dpkg -l | grep $i)" ]] && MISSINGPKG=$MISSINGPKG" "$i
	done
	[[ -z "$MISSING" ]] && echo -en "[OK]\n" || echo -en "[NOT OK]\n"
	return 0
}

# Install the packages passed as arguments.
# Don't ask for confirmation is the ASSUMEYES variable is set.
# Accept a list of packages as arguments.
install_packages() {
	[[ $# < 1 ]] && { echo "ERROR: no packages to install"; exit 1; }
	if [[ $ASSUMEYES ]]; then
		echo "The following packages are missing: " $MISSINGPKG
		read -r -p "Do you want to install them? [y/N] " input
		[[ "$input" != "y"  && "$input" != "Y" ]] && return 1
	fi
	type aptitude >/dev/null 2>&1 && PKGMNGR="aptitude" || PKGMNGR="apt-get"
	[[ $ASSUMEYES ]] && PKGMNGR=$PKGMNGR" -y"
	$PKGMNGR update && $PKGMNGR install $@
	return 0
}

# Check if the sources in the repository are newer than the installed deb.
# Accept 1 argument:
# $1 = name of the package to be checked.
# $2 = new version string to be checked (numbers, dashes  and dots).
deb_check_version() {
	[[ $# < 1 || $# > 2 ]] && { echo "ERROR: no arguments or too many."; exit 1; }
	OLDVER=$(dpkg -s $1 2>/dev/null | grep -Po '(?<=^Version:\s)[\d\.]*')
	[[ -z "$OLDVER" ]] && { echo "ERROR: non-existent package."; exit 1; }
	[[ -n "$2" && $2 =~ ^[0-9\.\-]*$ ]] || { echo "ERROR: version format not correct."; exit 1; }
	[[ -z "$2" ]] && { echo "Update repositories."; apt-get update; }
	echo -en "Checking version... "
	[[ -n "$2" ]] && NEWVER=$2 || NEWVER=$(apt-cache showsrc $1 | grep -Po '(?<=^Version:\s)[\d\.]*')
	[[ "$OLDVER" == "$NEWVER" ]] && { echo -en "[ALREADY UPDATED]\n"; exit 1; }
	echo -en "[NOT UPDATED]\n" && return 0
}

# Check if the sources in the remote git repo are newer than the local git repo.
# Accept 1 argument:
# $1 = directory where the local git repository to check is located.
git_check_version() {
	[[ $# != 1 ]] && { echo "ERROR: no arguments or too many."; exit 1; }
	echo -en "Checking version... "
	if [[ -d "./$1" ]]; then
		OLDDIR="$PWD" && cd "$1"
		git remote update >/dev/null 2>&1
		git diff --quiet || { echo -en "[ALREADY UPDATED]\n"; exit 1; }
		cd "$OLDDIR"
	fi; echo -en "[NOT UPDATED]\n" && return 0
}

# Update program sources from the designed debian repository.
# Accept 2 arguments:
# $1 = package whom sources have to be downloaded.
# $2 = apt-get options.
deb_update_sources() {
	[[ $# < 1 || $# > 2 ]] && { echo "ERROR: no arguments or too many.";  exit 1; }
	dpkg -s $1 >/dev/null 2>&1 || { echo "ERROR: non-existent package."; exit 1; }
	rm -rf "$1" && mkdir "$1" && cd "$1"
	echo "Download sources."
	apt-get $2 source $1
	return 0
}

# Update program sources from the desired git repository.
# Accept 1 argument:
# $1 = url of the git repository for the sources to be downloaded.
git_update_sources() {
	[[ $# != 1 ]] && { echo "ERROR: no arguments or too many."; exit 1; }
	NAME=$(echo "$1" | grep -Po '(?<=\/)[^\/]+(?=\.git)')
	[[ -z "$NAME" ]] && { echo "ERROR: wrong git url."; exit 1; }
	rm -rf "$NAME"
	echo "Download sources."
	git clone --recursive $1
	return 0
}
