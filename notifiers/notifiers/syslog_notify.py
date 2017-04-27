#!/usr/bin/python3
import json
import sys
import re
import syslog
import os

# Program can be called with a message UUID as argument or with no args
# It should be called by a symlink, and the parent dir to the symlink will hold the log level.
# For this notifier that writes to syslog it is only to write everything immediately to syslog,
# it is only called for levels in which the symlink exists.

DEBUG = False
SPOOLDIR = "/var/spool/kgp-notify/"


def dprint(mystring):
	if(DEBUG):
		print(mystring)

if __name__ == "__main__":

	if( len(sys.argv) > 2):
		dprint ("Only accepting no argument or message UUID as argument\n")
		sys.exit(1)

	if( len(sys.argv) == 2):
		message = sys.argv[1]
		p = re.compile(r'[a-zA-Z0-9-]{36}')
		if(p.search(message)): 
			try:
				f = open(os.path.join(SPOOLDIR, message), 'r')
				msg = json.loads(f.read())
				logentry = ("%s, [%s] %s") % (msg['levelText'],msg['issuer'],msg['message'])
				#dprint(logentry)
				syslog.syslog(int(msg['level']),logentry)
			except:
				dprint("Not a valid message")
				sys.exit(1)

		else:
			dprint("Not a valid message name")
			sys.exit(1)
	else:
		dprint("Nothing to do")
