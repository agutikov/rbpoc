#!/usr/bin/python


import socketserver
import json
from pprint import pprint
from sqlitedict import SqliteDict
import signal
import time


mydict = SqliteDict('./my_db.sqlite', autocommit=True)


recorded = 0

class MyUDPHandler(socketserver.BaseRequestHandler):
	
	"""
	This class works similar to the TCP handler class, except that
	self.request consists of a pair of data and client socket, and since
	there is no connection the client address must be given explicitly
	when sending data back via sendto().
	"""

	def handle(self):
		global recorded
		data = self.request[0].strip()
		socket = self.request[1]
#		print("{} wrote:".format(self.client_address[0]))
#		print(data)
#		socket.sendto(data.upper(), self.client_address)

		try:
			frame = json.loads(data.decode('utf-8'))
		except:
			pprint(data)
			print("LEN:", len(data))

		for sample in frame:
			mydict[sample[0]] = sample[1]
			recorded += 1
		
		print("recorded: ", recorded)
		
		


HOST, PORT = "0.0.0.0", 9999
server = socketserver.UDPServer((HOST, PORT), MyUDPHandler)


try:
	server.serve_forever()
except KeyboardInterrupt:
	server.server_close()
	mydict.close()













