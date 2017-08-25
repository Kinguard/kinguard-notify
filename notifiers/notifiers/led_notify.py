#!/usr/bin/python3
import json
import sys
import re
import os
import syslog
from subprocess import call
from subprocess import check_output

# Program can be called with a message UUID as argument or with no args
# It should be called by a symlink, and the parent dir to the symlink will hold the log level.

DEBUG = True

def setTrigger(ledpath,value):
	try:
		l = open(os.path.join(ledpath, "trigger"), 'w')
		l.write(value)
		l.close()
	except Exception as e:
		dprint(e)

def setBrightness(ledpath,value):	
	try:
		l = open(os.path.join(ledpath, "brightness"), 'w')
		l.write(value)
		l.close()
	except Exception as e:
		dprint(e)

def setDutyCycle(ledpath,value,period):
	try:
		l = open(os.path.join(ledpath, "delay_on"), 'w')
		if (value > 0):
			val = period * (value / 100)
			l.write("%.0f" % val)
		else:
			l.write(0)
		l.close()
		
		l = open(os.path.join(ledpath, "delay_off"), 'w')
		if (value > 0):
			val = period * (1 -(value / 100))
			l.write("%.0f" % val)
		else:
			l.write(0)
		l.close()

	except Exception as e:
		dprint(e)


def SetOn(ledpath):
	setTrigger(ledpath, "none")
	setBrightness(ledpath, "1")

def SetOff(ledpath):
	setTrigger(ledpath, "none")
	setBrightness(ledpath, "0")

def SetHeartbeat(ledpath):
	setTrigger(ledpath, "heartbeat")

def SetBlink(ledpath,duty = 50, period = 1000): 
	setTrigger(ledpath, "timer")
	setDutyCycle(ledpath,duty,period)

def dprint(mystring):
	if(DEBUG):
		print(mystring)

def PC():
	dprint("Running on PC, no LEDs to play with...")
	sys.exit(0)

def Opi():
	dprint("Running on OPI")
	green0 = "/sys/class/leds/opi:green:usr0/"
	green1 = "/sys/class/leds/opi:green:usr1/"
	green2 = "/sys/class/leds/opi:green:usr2/"
	red3 = "/sys/class/leds/opi:red:usr3/"

	activemsg = setPrioMsg(["sysctrl","kgp-control","kgp-backup"])
	if('level' in activemsg):
		if(activemsg['level'] <= syslog.LOG_ERR):
			print("LOG ERROR received")
			SetOn(red3)
			SetOn(green0)
			SetOn(green1)
			SetOff(green2)

		elif(activemsg['level'] <= syslog.LOG_WARNING):
			print("LOG WARNING received")
			SetHeartbeat(red3)
			SetOn(green0)
			SetOn(green1)
			SetOff(green2)

		elif(activemsg['level'] <= syslog.LOG_NOTICE):
			print("LOG NOTICE received")
			SetOff(red3)
			SetOn(green0)
			SetOn(green1)
			SetHeartbeat(green2)

		elif(activemsg['level'] <= syslog.LOG_INFO):
			print("LOG INFO received")
			SetOff(red3)
			SetOn(green0)
			SetBlink(green1,duty=50)
			SetOn(green2)

		elif(activemsg['level'] <= syslog.LOG_DEBUG):
			print("LOG DEBUG received")
			# do nothing for LED
		else:
			print("Unknown log level")
	else:
		# no level in active message set to default LED pattern
			print("No valid messages")
			SetOff(red3)
			SetOn(green0)
			SetOn(green1)
			SetOn(green2)


def KEEP():
	dprint("Running on KEEP")
	green = "/sys/class/leds/green/"
	blue = "/sys/class/leds/blue/"
	red = "/sys/class/leds/red/"

	activemsg = setPrioMsg(["sysctrl","kgp-control","kgp-backup"])
	if('level' in activemsg):
		if(activemsg['level'] <= syslog.LOG_ERR):
			print("LOG ERROR received")
			SetOn(red)
			SetOff(green)
			SetOff(blue)

		elif(activemsg['level'] <= syslog.LOG_WARNING):
			print("LOG WARNING received")
			SetOff(red)
			SetOff(green)
			SetHeartbeat(blue)

		elif(activemsg['level'] <= syslog.LOG_NOTICE):
			print("LOG NOTICE received")
			SetOff(red)
			SetHeartbeat(green)
			SetOff(blue)

		elif(activemsg['level'] <= syslog.LOG_INFO):
			print("LOG INFO received")
			SetOff(red)
			SetBlink(green,duty=50)
			SetOff(blue)

		elif(activemsg['level'] <= syslog.LOG_DEBUG):
			print("LOG DEBUG received")
			# do nothing for LED
		else:
			print("Unknown log level")
	else:
		# no level in active message set to default LED pattern
			print("No valid messages")
			SetOff(red)
			SetOn(green)
			SetOff(blue)

def setPrioMsg(issuer_prio = []):
	# if we need to have a priority on the messages based on issuer, a prio list must be supplied.
	
	SPOOLDIR = "/var/spool/kgp-notify/"
	curr_lvl = 99;  # set initially to a low priority level.
	curr_issuer = 99
	currmsg = ""

	dprint("Reading messages")
	p = re.compile(r'[a-zA-Z0-9-]{36}')
	for message in os.listdir(SPOOLDIR):
		if(not p.search(message)): 
			continue
		else:
			try:
				f = open(os.path.join(SPOOLDIR, message), 'r')
				msg = json.loads(f.read())
			except Exception as e:
				dprint(e)
				dprint("Failed to read message\n")

			if( (int(msg['level']) == curr_lvl) and len(issuer_prio) ):
				try:
					issuer = issuer_prio.index(msg['issuer'])
					if (issuer < curr_issuer):
						curr_issuer = issuer
						currmsg = message
				except:
					dprint("Issuer '"+msg['issuer']+"' not in prio list")
				
			if(int(msg['level']) < curr_lvl):
				currmsg = message
				curr_lvl = int(msg['level'])
	
	
	msg = {}
	if(currmsg != ""):
		try:
			f = open(os.path.join(SPOOLDIR, currmsg), 'r')
			msg = json.loads(f.read())
		except:
			print("No valid messages")

	return msg



if __name__ == "__main__":

	platform = {'PC' : PC,
				'Opi': Opi,
				'Armada' : KEEP
				}

	if( len(sys.argv) > 2):
		dprint ("Only accepting no argument or message UUID as argument\n")
		sys.exit(1)

	try:
		# Get what plaform we are running on
		jsonmsg = check_output("kgp-sysinfo -t",shell=True)
		sysinfo = json.loads(jsonmsg.decode("utf-8"))['systype']

	except Exception as e:
		dprint(e)
		dprint("Failed to get sysinfo\n")
		sys.exit(1)

	# Call appropriate platform functions
	try:
		platform[sysinfo['typeText']]()

	except Exception as e:
		dprint(e)
		dprint("Platform '"+sysinfo['typeText']+"' not supported")
		sys.exit(1)
