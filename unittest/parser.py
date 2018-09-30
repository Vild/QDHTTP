#!/usr/bin/python

import sys
import re

if len(sys.argv) != 2:
	print "usage: ", sys.argv[0], " <input file>"
	exit(1)
	
result = sys.argv[1].split('/')[2]

l = False
t = False
m = False
d = False
s = False
	
resp_line = re.compile("HTTP/1.[0|1] ([0-9]{3}) .*")
length = re.compile("Content-Length: ([0-9]+)")
typ = re.compile("Content-Type: (.+)\r\n")
date = re.compile("Date: (.+)\r\n")
mod = re.compile("Last-Modified: (.+)\r\n")
server = re.compile("Server: (.+)\r\n")
	
len_str = "-"
typ_str = "-"
date_str = "-"
server_str = "-"
mod_str = "-"
	
with open(sys.argv[1], 'r') as f:
	count = 0;
	for line in f:
		if count == 0:
			match = re.match(resp_line, line)
			if match == None:
				result += "\tERROR: invalid response line: " + line[:-1]
			else:
				result += "\t" + match.group(1)
		else:
			match = re.match(length, line)
			if match != None:
				l = True
				len_str = match.group(1)
			match = re.match(typ, line)
			if match != None:
				t = True
				typ_str = match.group(1)
			match = re.match(date, line)
			if match != None:
				d = True
				date_str = match.group(1)
			match = re.match(mod, line)
			if match != None:
				m = True
				mod_str = match.group(1)
			match = re.match(server, line)
			if match != None:
				s = True
				server_str = match.group(1)
		count += 1

if l or t or m or s:
	result += "\t"
	
if l:
	result += "L"	
if s:
	result += "S"
if t:
	result += "T"
if m:
	result += "M"
if d:
	result += "D"

		
print result, '\t', len_str, '\t', typ_str, '\t', date_str, '\t', mod_str, '\t', server_str
