#!/bin/sh
LEVEL=$( basename $(dirname $0))
echo $filename
echo "Notifier '1' called on level "${LEVEL}", Arg list:" $@ >> /tmp/notifier.log
exit 0

