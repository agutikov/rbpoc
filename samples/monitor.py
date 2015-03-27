#!/usr/bin/python2.7


import time
import mraa
from pprint import pprint
import thread
import Queue
import socket
import sys
import json

adc0 = mraa.Aio(2)

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
	
# samples = get_aio_frame(adc0, 1000, 500)
# pprint(samples)
# print(json.dumps(samples))
# exit()

frame_queue = Queue.Queue()


def monitor_thread_routine (adc, q):
	while True:
		q.put(get_aio_frame(adc, 250, 5000))


HOST = sys.argv[1]
PORT = int(sys.argv[2])

print("%s:%d" % (HOST, PORT))

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def uploader_routine (q, s, host, port):
	sended = 0
	while True:
		frame = q.get(block=True)
		s.sendto(json.dumps(frame), (host, port))
		sended += len(frame)
		print("sended: ", sended)


try:
	thread.start_new_thread( monitor_thread_routine, (adc0, frame_queue) )
	thread.start_new_thread( uploader_routine, (frame_queue, sock, HOST, PORT) )
except:
	print "Error: unable to start thread"



while True:
	time.sleep(1.0)




