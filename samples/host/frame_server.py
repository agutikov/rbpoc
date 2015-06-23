#!/usr/bin/python3.3


import socketserver
import json
from pprint import pprint
import signal
import time
import sys


recorded = 0

class MyTCPHandler(socketserver.StreamRequestHandler):
	
	"""
	This class works similar to the TCP handler class, except that
	self.request consists of a pair of data and client socket, and since
	there is no connection the client address must be given explicitly
	when sending data back via sendto().
	"""

	def handle(self):
		global recorded
		
		while True:
			data = self.rfile.readline().strip()
				
			if len(data) == 0:
				break
			
			try:
				msg = json.loads(data.decode('utf-8'))
			except:
				pprint(msg)
				print("LEN:", len(msg))

			print(msg)

#			for frame in msg:
				
				

HOST = "0.0.0.0"
PORT = int(sys.argv[1])

print("Listen to %s:%d" % (HOST, PORT))

server = socketserver.TCPServer((HOST, PORT), MyTCPHandler)


try:
	server.serve_forever()
except KeyboardInterrupt:
	server.server_close()













