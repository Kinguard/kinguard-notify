#!/usr/bin/python3
import json
import sys
import re
import os
import smtplib
import configparser
from email.mime.text import MIMEText

# Program can be called with a message UUID as argument or with no args
# It should be called by a symlink, and the parent dir to the symlink will hold the log level.
# For this notifier that an email it is only to write everything immediately to syslog,
# it is only called for levels in which the symlink exists.

DEBUG = False
SPOOLDIR = "/var/spool/kgp-notify/"
SYSINFO = "/etc/opi/sysinfo.conf"


def dprint(mystring):
	if(DEBUG):
		print(mystring)

def add_section_header(properties_file, header_name):
	# configparser.ConfigParser requires at least one section header in a properties file.
	# Our properties file doesn't have one, so add a header to it on the fly.
	yield '[{}]\n'.format(header_name)
	for line in properties_file:
		yield line


if __name__ == "__main__":

	if( len(sys.argv) > 2):
		dprint ("Only accepting no argument or message UUID as argument\n")
		sys.exit(1)

	if( len(sys.argv) == 2):

		try:
			fh_sysconf = open(SYSINFO, encoding="utf_8")
		except Exception as e:
			dprint(e)
			print("Error opening SYSINFO file: "+SYSINFO)

		sysconf = configparser.ConfigParser()
		# There are no sections in our ini files, so add one on the fly.
		try:
			sysconf.read_file(add_section_header(fh_sysconf, 'sysinfo'), source=SYSINFO)
			sysinfo = sysconf['sysinfo']
			if 'opi_name' not in sysinfo:
				# update dns by using serialnumber in flash
				dprint("Missing opi_name in sysinfo")
				sys.exit(1)
			else:
				opi_name = sysinfo['opi_name'].strip('"')

		except Exception as e:
			dprint(e)
			print("Error parsing sysconfig")
			sys.exit(1)

		message = sys.argv[1]
		p = re.compile(r'[a-zA-Z0-9-]{36}')
		if(p.search(message)): 
			try:
				f = open(os.path.join(SPOOLDIR, message), 'r')
				msg = json.loads(f.read())

			except Exception as e:
				dprint(e)
				print("Not a valid message")
				sys.exit(1)

			try:
				body = ("Message Severity: %s\n"
						"Message Issuer: %s\n\n"
						"Message Body: \n%s\n"
					) % (msg['levelText'],msg['issuer'],msg['message'] )
				sender = ("root@%s.op-i.me") % (opi_name)
				receiver = "root@localhost"
				email = MIMEText(body)
				email['Subject'] = ("'%s' message from %s") % (msg['levelText'],opi_name)
				email['From'] = sender
				email['To'] = receiver

				s = smtplib.SMTP('localhost')
				s.sendmail(sender, receiver, email.as_string())
				s.quit()
			except Exception as e:
				dprint(e)
				print("Failed to send message")
				sys.exit(1)


		else:
			dprint("Not a valid message name")
			sys.exit(1)
	else:
		dprint("Nothing to do")
