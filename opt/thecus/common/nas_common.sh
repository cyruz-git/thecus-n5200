#!/bin/bash
# Common functions for nas management.
# --------------------------------
# cyruz - http://ciroprincipe.info

BASE_NAME=${0##*/}
ICH4_GPIO=/proc/thecus_io
STOP_FILE=/var/run/nas_stop
LOCK_FILE=/var/run/nas_lock
LOG_FILE=/var/log/$BASE_NAME.log
LCD_MNGR=/opt/thecus/nas_lcd
LED_MNGR=/opt/thecus/nas_led
SYS_DOWN=/opt/thecus/nas_sysdown
EMAIL_ADDR="focabresm@gmail.com"

# Check if the script is run as root and if required tools are present.
# Accept a list of tools as arguments.
preliminary_check() {
	write_log -t "Checking root... "
	[[ $(id -u) -ne 0 ]] && { write_log "[NOT OK]\n"; exit 1; }
	write_log "[OK]\n" && write_log -t "Checking required tools... "
	for i in $@; do
		type $i >/dev/null 2>&1 || MISSING=$i" "$MISSING
	done; [[ -z $MISSING ]] && write_log "[OK]\n" || write_log "[NOT OK] MISSING: $MISSING\n"
}

# Check if stop or lock file are present.
running_check() {
	write_log -t "Checking running locks... "
	[[ -f $STOP_FILE ]] && ERRMSG="Stop file present, aborting."
	[[ -f $LOCK_FILE ]] && ERRMSG="Lock file present (PID $(< $LOCK_FILE) running), aborting."
	[[ -z $ERRMSG ]] && write_log "[OK]\n" || { write_log "[NOT OK] $ERRMSG\n"; exit 1; }
}

# Create the lock file with script's pid.
create_lock() {
	write_log -t "Creating lock file... "
	echo $$ > $LOCK_FILE
	write_log "[OK]\n"
}

# Remove the lock file.
remove_lock() {
	write_log -t "Removing lock file... "
	rm -f $LOCK_FILE
	write_log "[OK]\n"
}

# Create the stop file.
create_stop() {
	write_log -t "Creating stop file... "
	touch $STOP_FILE
	write_log "[OK]\n"
}

# Remove the stop file.
remove_stop() {
	write_log -t "Removing stop file... "
	rm -f $STOP_FILE
	write_log "[OK]\n"
}

# Toggle led status.
# Require 1 argument: led name.
toggle_led() {
	if [[ -e $ICH4_GPIO ]]; then
		[[ $# != 1 ]] && { echo "ERROR: no argument or too many."; return; }
		[[ ! $1 =~ busy|copy|fail ]] && { echo "ERROR: wrong argument."; return; }
		write_log -t "Toggling $1 led... "
		$LED_MNGR $1
		write_log "[OK]\n"
	fi
}

# Notify a message by mail.
# Require 2 arguments: subject and body.
mail_notify() {
	[[ $# != 2 ]] && { echo "ERROR: no arguments or too many."; return; }
	echo "$2" | mail -s "$1" "$EMAIL_ADDR"
}


# Write log file with an optional timestamp.
# Accept a series of arguments as a log record.
# If 1st argument is "-t", adds a timestamp to the log record.
write_log() {
	if [[ $# > 0 && $# < 3 ]]; then
		[[ $1 == "-t" ]] && echo -en "[$(date '+%F %H:%M:%S')] $BASE_NAME :: ${@:2}" || echo -en "$@"
	fi
}

# Redirect output to log file.
redirect_out() {
	exec >> $LOG_FILE
}

