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
/etc/crontab
/etc/fstab
/etc/lilo.conf
/etc/mailname
/etc/minidlna.conf
/etc/smartd.conf
/etc/sudoers
/etc/apt/sources.list
/etc/default/smartmontools
/etc/logrotate.d/nas_backup
/etc/nullmailer/*
/etc/proftpd/proftpd.conf
/etc/rsyslog.d/smartd.conf
/etc/samba/smb.conf
/opt/thecus/*
"
DESTDIR="$HOMEDIR/thecus-n5200/"

rsync -RLav $SOURCEFILES $DESTDIR
chown -R $USR:$USR $DESTDIR

