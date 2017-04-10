#!/usr/bin/env python

import os

#with open('rand.csv', w) as file:
#for program in ['scan', 'sort', 'focus']:
program = 'focus'
n = 3
while n <= 100:
	command = './virtmem 100 ' + str(n) + ' custom ' + program
	os.system(command)
	n+=1



#for program in ['scan', 'sort', 'focus']:
#	n = 3
#	while n <= 100:
#		command = './virtmem 100 ' + str(n) + ' fifo ' + program


#for program in ['scan', 'sort', 'focus']:
#	n = 3
#	while n <= 100:
#		command = './virtmem 100 ' + str(n) + ' custom ' + program





#for algorithm in ['rand', 'fifo', 'custom']:
#	n = 3
#	while n <= 100:
#		command = './virtmem 100 ' + n + ' ' + algorithm + ' scan'


#for algorithm in ['rand', 'fifo', 'custom']:
#	n = 3
#	while n <= 100:
#		command = './virtmem 100 ' + n + ' ' + algorithm + ' sort'


#for algorithm in ['rand', 'fifo', 'custom']:
#	n = 3
#	while n <= 100:
#		command = './virtmem 100 ' + n + ' ' + algorithm + ' focus'
