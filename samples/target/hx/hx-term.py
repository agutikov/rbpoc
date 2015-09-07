#!/usr/bin/python

from bluetooth import *
import sys
import select
import binascii
from pprint import pprint


if sys.version < '3':
    input = raw_input

addr = "00:12:F3:23:50:5E"

# search for the SampleServer service
uuid = "00001101-0000-1000-8000-00805f9b34fb"
service_matches = []

while True:
    service_matches = find_service( uuid = uuid, address = addr )
    if len(service_matches) > 0:
        break
    print("couldn't find the SampleServer service =(")
    print("try again...")


first_match = service_matches[0]
port = first_match["port"]
name = first_match["name"]
host = first_match["host"]

print("connecting to \"%s\" on %s" % (name, host))

# Create the client socket
sock=BluetoothSocket( RFCOMM )
sock.connect((host, port))

print("connected.  type stuff")
print("...\n")

sock.setblocking(0)






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




while True:
    while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
        line = sys.stdin.readline()
        if line:
            data = line[:-1]
            data = data.decode('string-escape')
            data += '\r'
            print("Send: " + binascii.hexlify(data))
            sock.send(data)
    while sock in select.select([sock], [], [], 0)[0]:
        data = sock.recv(100)
        # print("Rcvd: " + binascii.hexlify(data))
        hx_buffer = hx_buffer + data
        packet = get_parse_single_packet()
        while packet:
            print("Rcvd: " 
                    + ("0x%02X 0x%02X" % packet[0]) 
                    + (" \"%c%c\" " % packet[0]) 
                    + (" - %3d " % len(packet[1])) 
                    + str(packet[1]))
            packet = get_parse_single_packet()



print("Exit.")

sock.close()










