#!/bin/sh
FROM_DIR=./locale
MIME_SIG=takamatsu-scooby
MV=/bin/mv
MKDIR=/bin/mkdir
DEST_DIR=/boot/home/config/locale

alert --info "Would you like to install message catalogs?" Cancel OK > /dev/nulL
case $? in
0)
	exit
	;;
1)
	$MKDIR $DEST_DIR
	$MV -f $FROM_DIR $DEST_DIR/$MIME_SIG
	alert --idea "Installing have been finished. Add \"export LANGUAGE=locale name\" to ~/config/boot/UserSetupEnvironment" OK > /dev/null
	;;
esac
exit
