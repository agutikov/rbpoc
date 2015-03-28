#!/usr/bin/python


import socketserver
import json
from pprint import pprint
from sqlitedict import SqliteDict
import signal
import time


mydict = SqliteDict('./my_db.sqlite', autocommit=False)


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
				frame = json.loads(data.decode('utf-8'))
			except:
				pprint(data)
				print("LEN:", len(data))

			for sample in frame:
				mydict[float(sample[0])] = int(sample[1])
				recorded += 1

			try:
				mydict.commit()
				print("recorded: ", recorded)
			except:
				print("dropped: ", recorded)
				mydict.conn.rollback()
				
			
'''
			for sample in frame:
				mydict[sample[0]] = sample[1]
				recorded += 1
'''


HOST, PORT = "0.0.0.0", 9999
server = socketserver.TCPServer((HOST, PORT), MyTCPHandler)


try:
	server.serve_forever()
except KeyboardInterrupt:
	server.server_close()
	mydict.close()













