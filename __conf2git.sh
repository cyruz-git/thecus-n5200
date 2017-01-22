#!/bin/bash
# Archives files for the thecus-n5200 repository.
# --------------------------------
# cyruz - http://ciroprincipe.info

# root check
[[ $(id -u) -ne 0 ]] && { echo "Please run this script as root!"; exit 1; }

# rsync check
type rsync > /dev/null 2>&1 || { echo >&2 "Error: rsync not installed. Aborting."; exit 1; }

USR="cyrus"
HOMEDIR="/home/$USR"
SOURCEFILES="
/etc/apt/sources.list
/etc/aria2/aria2.conf
/etc/crontab
/etc/default/mathopd
/etc/default/smartmontools
/etc/default/syncthing
/etc/default/transmission-daemon
/etc/fstab
/etc/init.d/aria2
/etc/init.d/transmission-daemon
/etc/init.d/syncthing
/etc/lilo.conf
/etc/logrotate.d/freedns-updater
/etc/logrotate.d/nas_*
/etc/mailname
/etc/mathopd.conf
/etc/minidlna.conf
/etc/network/interfaces
/etc/nullmailer/
/etc/nut/
/etc/proftpd/proftpd.conf
/etc/rsyslog.d/smartd.conf
/etc/samba/smb.conf
/etc/smartd.conf
/etc/snapraid.conf
/etc/sudoers
/etc/transmission/settings.json
/etc/udev/rules.d/10-powerwalker.rules
/opt/bin/
/opt/build/common/
/opt/build/update-*
/opt/thecus/
"
DESTDIR="$HOMEDIR/thecus-n5200/"

rsync -RLav --delete $SOURCEFILES $DESTDIR
chown -R $USR:$USR $DESTDIR

