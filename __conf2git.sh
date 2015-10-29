#!/bin/bash
# Archives files for the thecus-n5200 repository.
# --------------------------------
# cyruz - http://ciroprincipe.info

# root check
if [[ $(id -u) -ne 0 ]]; then
	echo "Please run this script as root!"
	exit 1
fi

# rsync check
type rsync > /dev/null 2>&1 || { echo >&2 "Error: rsync not installed. Aborting."; exit 1; }

USR="cyrus"
HOMEDIR="/home/$USR"
SOURCEFILES="
/etc/aria2/aria2.conf
/etc/crontab
/etc/fstab
/etc/lilo.conf
/etc/mailname
/etc/mathopd.conf
/etc/minidlna.conf
/etc/smartd.conf
/etc/nut/
/etc/sudoers
/etc/apt/sources.list
/etc/default/mathopd
/etc/default/pyload
/etc/default/smartmontools
/etc/default/syncthing
/etc/default/transmission-daemon
/etc/init.d/aria2
/etc/init.d/transmission-daemon
/etc/init.d/pyload
/etc/init.d/syncthing
/etc/logrotate.d/freedns-updater
/etc/logrotate.d/nas_backup
/etc/network/interfaces
/etc/nullmailer/
/etc/proftpd/proftpd.conf
/etc/pyload/pyload.conf
/etc/rsyslog.d/smartd.conf
/etc/transmission/settings.json
/etc/samba/smb.conf
/etc/udev/rules.d/10-powerwalker.rules
/opt/bin/
/opt/build/update-*
/opt/thecus/
"
DESTDIR="$HOMEDIR/thecus-n5200/"

rsync -RLav --delete $SOURCEFILES $DESTDIR
chown -R $USR:$USR $DESTDIR

