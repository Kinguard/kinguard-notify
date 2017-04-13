#!/bin/sh
LEVEL=$( basename $(dirname $0))
echo $filename
echo "Notifier 'fail' called on level "${LEVEL}", Arg list:" $@ >> /tmp/notifier.log
exit 1

