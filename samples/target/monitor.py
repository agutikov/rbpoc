#!/usr/bin/python2.7


import time
import mraa
from pprint import pprint
import thread
import Queue
import socket
import sys
import json



start_time = time.time()

def get_aio_frame(adc, count, wait_us):
	global start_time
	frame = []
	prev = time.time()
	while count:
		t = time.time()
		while (t - prev)*1000000 < wait_us:
			t = time.time()
#		frame.append([t - start_time, adc.read()])
		frame.append([t, adc.read()])
		prev = t
		count -= 1
		
	return frame

def get_aio_frame_wo_time(adc, count, wait_us):
	global start_time
	frame = []
	prev = time.time()
	wait_s = wait_us * 0.000001
	while count:
		t = time.time()
		while (t - prev) < wait_s:
			t = time.time()
		frame.append(adc.read())
		prev = t
		count -= 1
		
	return frame


frame_queue = Queue.Queue()


def monitor_thread_routine (q):
	print("monitor_thread_routine")
	adc = mraa.Aio(1)
	print("adc on")
	while True:
		q.put(get_aio_frame_wo_time(adc, 2000, 500))
		time.sleep(0.1)
		pass

HOST = sys.argv[1]
PORT = int(sys.argv[2])

print("%s:%d" % (HOST, PORT))

def uploader_routine (q, host_port):
	sended = 0
	frame = []
	while True:
		try:
			sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			sock.connect(host_port)
			connected = True
		except:
			time.sleep(1)
			print("not connected")
			connected = False
		while connected:
			tail = q.get(block=True)
			frame += tail
			try:
				sock.sendall(json.dumps(frame) + "\n")
				sended += len(frame)
				print("sended: %d" % sended)
				frame = []
			except:
				print("tcp connect lost")
				connected = False


try:
	thread.start_new_thread( monitor_thread_routine, (frame_queue, ) )
	print("monitor_thread_routine started")	
	thread.start_new_thread( uploader_routine, (frame_queue, (HOST, PORT)) )
	print("uploader_routine started")	
except:
	print "Error: unable to start thread"



while True:
	time.sleep(1.0)




