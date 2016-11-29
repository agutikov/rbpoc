#!/usr/bin/python2.7


import time
import mraa
from pprint import pprint
import thread
import Queue
import socket
import sys
import json
import csv
from fcntl import ioctl
import struct
import array
from datetime import datetime


ADS1015_IOCTL_SWITCH = 0xC004730D

FRAME_DESCRIPTOR_FORMAT = "QQLL"

def get_frame(adc_file):
	frame = []
	buf = array.array('L', [0, 0, 0, 0, 0, 0])
	try:
		ioctl(adc_file, ADS1015_IOCTL_SWITCH, buf, 1)
		desc = struct.unpack(FRAME_DESCRIPTOR_FORMAT, buf);
#		pprint(desc)
		data_arr = array.array('h')
		data_arr.fromfile(adc_file, desc[2])
#		print("readed", len(data))
		data = data_arr.tolist()
		frame = [[desc[0], desc[1]], data]
	except IOError, err:
		pprint(err)
	return frame




frame_queue = Queue.Queue()

run = True
source_stopped = False
sink_stopped = False


def monitor_thread_routine ():
	global frame_queue
	global source_stopped
	global run
	print("monitor_thread_routine started")
	while run:
		try:
			print("try to start adc")
			with open("/dev/ADS1015_ADC", "rb") as adc_file:
				print("adc on")
				time.sleep(0.1)
				while run:
					frame = get_frame(adc_file)
	#				pprint(frame)
					if len(frame) == 2:
						frame_queue.put(frame)
	#				print(frame_queue.qsize())
					if frame_queue.qsize() > 100:
						print("queue size %d - drop frames" % frame_queue.qsize())
						while frame_queue.qsize() > 50:
							frame_queue.get(block=False)
						print("queue size %d" % frame_queue.qsize())
					else:
						time.sleep(0.2)
		except IOError, err:
			pprint(err)

		time.sleep(0.5)
		
	source_stopped = True
	print("monitor_thread_routine stopped")



def uploader_routine ():
	global frame_queue
	global run
	global sink_stopped
	
	HOST = sys.argv[1]
	PORT = int(sys.argv[2])
	print("host:port == %s:%d" % (HOST, PORT))

	sended = 0
	print("uploader_routine started")
	while run:
		try:
			sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			sock.connect(host_port)
			connected = True
		except:
			time.sleep(1)
			print("not connected")
			connected = False
		while connected:
			frame = frame_queue.get(block=True)
			msg = json.dumps(frame) + "\n"
#			print(msg)
			try:
				sock.sendall(msg)
				sended += len(frame[1])
				print("values sended: %d" % sended)
			except:
				print("tcp connect lost")
				connected = False
	sink_stopped = True
	print("uploader_routine stopped")


def writer_routine ():
	global frame_queue
	global run
	global sink_stopped

	if len(sys.argv) == 2:
		output_file_basename = sys.argv[1]
	else:
		output_file_basename = "./output"
		
	saved = 0
	
	print("writer_routine started")
	while run:
		try:
			filename = output_file_basename + "_" + datetime.now().strftime("%m.%d-%H:%M:%S") + ".txt"
			print("open file " + filename)
			f = open(filename, "w+")
			msg = ""
			connected = True
			while connected:
				frame = frame_queue.get(block=True)
				msg = msg + "\n".join(map(str, frame[1]))
				try:
					f.write(msg)
					f.flush()
					msg = "\n"
					saved += len(frame[1])
					# print("values saved: %d" % saved)
					if saved > 60*920:
						f.close()
						connected = False
						saved = 0
				except Exception as e:
					print(e)
					time.sleep(1)
					connected = False
		except Exception as e:
			print(e)
			time.sleep(1)
	sink_stopped = True
	print("writer_routine stopped")



try:
	thread.start_new_thread( monitor_thread_routine )
	print("source thread started")
	if len(sys.argv) == 3:
		thread.start_new_thread( uploader_routine )
	else:
		thread.start_new_thread( writer_routine )
	print("sink thread started")	
except:
	print "Error: unable to start thread"


try:
	while True:
		time.sleep(1.0)
except KeyboardInterrupt:
	run = False
	while not sink_stopped and not source_stopped:
		time.sleep(0.1)
	
	



