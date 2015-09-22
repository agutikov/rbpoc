#!/usr/bin/python

from bluetooth import *
import sys
import select
import binascii
from pprint import pprint
import time
import datetime
import struct
import thread
import Queue
import socket
import json
from fcntl import ioctl
import array


if sys.version < '3':
    input = raw_input

HX_BT_MAC_ADDR = "00:12:F3:23:50:5E"

# search for the SampleServer service
SPP_DEFAULT_UUID = "00001101-0000-1000-8000-00805f9b34fb"

def hx_connect (addr, uuid):
    global run
    service_matches = []

    while run:
        service_matches = find_service( uuid = uuid, address = addr )
        if len(service_matches) > 0:
            break
        print("couldn't find the SampleServer service =(")
        print("try again...")
        time.sleep(0.5)
    else:
        return None

    first_match = service_matches[0]
    port = first_match["port"]
    name = first_match["name"]
    host = first_match["host"]

    print("connecting to \"%s\" on %s" % (name, host))

    # Create the client socket
    sock=BluetoothSocket( RFCOMM )
    sock.connect((host, port))

    print("BT connected")

    sock.setblocking(0)

    return sock

proto = { True: 
    {
        0x63 : {
            0x20 : ["<5B16H", 5 + 16*2],
            0x10 : ["<5B16H", 5 + 16*2],

            0x12 : ["<6B", 6],
            0x13 : ["<6B", 6],
            0x16 : ["<6B", 6],
            0x21 : ["<6B", 6],
            0x22 : ["<6B", 6],
            0x23 : ["<6B", 6],
            0x24 : ["<6B", 6],
            0x25 : ["<6B", 6],
            0x31 : ["<6B", 6],
            0x34 : ["<6B", 6],
            0x35 : ["<6B", 6],

            0xd0 : ["80s", 80],
            0xd1 : ["<10B", 10],
            0xd5 : ["<10B", 10],

            0xf1 : ["<6B", 6],
            0xf2 : ["<6B", 6],
            0xf3 : ["<6B", 6],
            0xf4 : ["<6B", 6],
            0xf5 : ["<6B", 6],
            0xf6 : ["<6B", 6],
            0xf7 : ["<6B", 6],

        },
        0x3f : {
            0x24 : ["<24Bc", 25],
            0x43 : ["<Bc", 2],
            0x5e : ["<8Bc", 9],
            0x63 : ["<64Bc", 65],
            0x67 : ["<Bc", 2],
            0x68 : ["<6Bc", 7],
            0x69 : ["", -1],
            0x6c : ["<1Bc", 2],
            0x75 : ["<4Bc", 5],
            0x78 : ["<1Bc", 2]
        },
        0x21 : {
            0x44 : ["c", 1],
            0x64 : ["c", 1],
            0x65 : ["c", 1]
        }
    },
    False: {
        0x3f : {
            0x24 : ["c", 1],
            0x43 : ["c", 1],
            0x5e : ["c", 1],
            0x63 : ["c", 1],
            0x68 : ["c", 1],
            0x69 : ["c", 1],
            0x6c : ["c", 1],
            0x75 : ["c", 1],
            0x78 : ["c", 1],
        },
        0x21 : {
            0x44 : ["<4Bc", 5],
            0x64 : ["<5Bc", 6],
            0x65 : ["<4Bc", 5],
        }
    }
}

hx_buffer = b''

def get_parse_single_packet ():
    global hx_buffer

    #print("Buffer: " + binascii.hexlify(hx_buffer))
    # if packet contains header
    if len(hx_buffer) > 2:
        cmd1 = ord(hx_buffer[0])
        cmd2 = ord(hx_buffer[1])
        if cmd1 in proto[True] and cmd2 in proto[True][cmd1]:
            fmt = proto[True][cmd1][cmd2][0]
            fmt_len = proto[True][cmd1][cmd2][1]

            #print(cmd1, cmd2)

            # if packet contains data
            if fmt_len < 0 or len(hx_buffer) >= fmt_len + 2:
                if fmt_len >= 0:
                    # fixed length
                    packet = ((cmd1, cmd2), struct.unpack(fmt, hx_buffer[2:2 + fmt_len]))
                    hx_buffer = hx_buffer[2 + fmt_len:]
                else:
                    # variable length
                    packet = ((cmd1, cmd2), hx_buffer[2:])
                    hx_buffer = b''

                return packet

            elif fmt_len > 0 and len(hx_buffer) < fmt_len + 2:
                pass
            else:
                hx_buffer = b''
        else:
            hx_buffer = b''

    return None

##############################################################################

frame_queue = Queue.Queue()

run = True
monitor_stopped = False
uploader_stopped = False


def monitor_thread_routine (q):
    global monitor_stopped
    global run
    global hx_buffer

    frame = [[0, 0], []]

    print("monitor_thread_routine started")
    while run:
        try:
            print("try to connect via BT")

            sock = hx_connect(HX_BT_MAC_ADDR, SPP_DEFAULT_UUID)

            if not sock:
                continue

            sock.send("!d0000\x00\r")
            sock.send("!e0000\r")
            last_e_time = datetime.datetime.now()

            while run:

                if datetime.datetime.now() - last_e_time >= datetime.timedelta(seconds=59):
                    sock.send("!d0000\x00\r")
                    sock.send("!e0000\r")
                    last_e_time = datetime.datetime.now()
                while sock in select.select([sock], [], [], 0)[0]:
                    data = sock.recv(100)
                    # print("Rcvd: " + binascii.hexlify(data))
                    hx_buffer = hx_buffer + data
                    packet = get_parse_single_packet()
                    while packet:
                        if packet[0][0] == 0x63:
                            if packet[0][1] == 0x10:
                                samples = packet[1][5:21]
                                frame[1] += samples
                                #pprint(samples)
                            if packet[0][1] == 0x20:
                                samples = packet[1][5:21]
                                samples_1 = samples[::2]
                                samples_2 = samples[1::2]
                               # frame[1] += samples_1
                                #pprint(samples_1)
                                #pprint(samples_2)
                            '''
                            print("[" + datetime.datetime.now().strftime("%H:%M:%S.%f") + "] Rcvd: "
                                    + ("0x%02X 0x%02X" % packet[0]) 
                                    + (" \"%c%c\" " % packet[0]) 
                                    + (" - %3d " % len(packet[1])) 
                                    + str(packet[1]))
                            '''
                        packet = get_parse_single_packet()

#               pprint(frame)
                if len(frame) == 2 and len(frame[1]) > 0:
                    q.put(frame)
                    frame = [[0, 0], []]
#               print(q.qsize())
                if q.qsize() > 100:
                    print("queue size %d - drop frames" % q.qsize())
                    while q.qsize() > 50:
                        q.get(block=False)
                    print("queue size %d" % q.qsize())
                else:
                    time.sleep(0.2)

            else: # while run
                sock.close()

        except IOError, err:
            pprint(err)

        time.sleep(0.5)

    monitor_stopped = True
    print("monitor_thread_routine stopped")


HOST = sys.argv[1]
PORT = int(sys.argv[2])

print("host:port == %s:%d" % (HOST, PORT))

def uploader_routine (q, host_port):
    global run
    global uploader_stopped
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
            frame = q.get(block=True)
            msg = json.dumps(frame) + "\n"
#           print(msg)
            try:
                sock.sendall(msg)
                sended += len(frame[1])
                print("frames sended: %d" % sended)
            except:
                print("tcp connect lost")
                connected = False
    uploader_stopped = True
    print("uploader_routine stopped")


try:
    thread.start_new_thread( monitor_thread_routine, (frame_queue, ) )
    print("monitor thread started") 
    thread.start_new_thread( uploader_routine, (frame_queue, (HOST, PORT)) )
    print("uploader thread started")    
except:
    print "Error: unable to start thread"


try:
    while True:
        time.sleep(1.0)
except KeyboardInterrupt:
    run = False
    while not uploader_stopped and not monitor_stopped:
        time.sleep(0.1)
    
    







