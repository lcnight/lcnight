#!/usr/bin/python
#coding=utf-8

## common mail module
def Usage():
    print '''Usage: mail.py <subject> <content> [ addr list (sep by ,) ]
    content in text/html MIME format
    '''
    sys.exit(0)

import time
import smtplib
import urllib2
import sys

if len(sys.argv) < 3:
    Usage();

mailsubject = sys.argv[1]
mailcontent = sys.argv[2]
monitor_time=time.ctime()
message = """From: monitor <monitor@taomee.com>
To: related persons
MIME-Version: 1.0
Content-type: text/html
Subject: [%s] monitor mail: %s

<h1>monitor mail</h1>
%s
<br/> <br/>
monitor time: %s
""" %(monitor_time, mailsubject, mailcontent, monitor_time)

sender = 'itl_alarm@taomee.com'
receivers = ['lc@taomee.com']
if len(sys.argv) > 3 :
    addrlist=sys.argv[3]
    receivers = addrlist.split(',')

def sendmail():
    try:
        smtpObj = smtplib.SMTP('mail.taomee.com')
        smtpObj.sendmail(sender, receivers, message)
        print "Successfully sent email"
    except SMTPException:
        print "Error: unable to send email"

###--------------- mail logic -------------
try:
    sendmail()
except urllib2.URLError,e:
    print "error: %r" %e
    sendmail()
