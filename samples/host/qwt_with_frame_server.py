#!/usr/bin/env python


import json
from pprint import pprint
import signal
import time
import sys
# from threading import Thread
import thread

from PyQt4 import Qt
import PyQt4.Qwt5 as Qwt
# from PyQt4.Qwt5.anynumpy import *
from numpy import *

import SocketServer




recorded = 0

frames = []

class MyTCPHandler(SocketServer.StreamRequestHandler):
	
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
				pprint(frame)
				print("LEN:", len(frame))

#			print(frame)

			'''
				frame[0][0] - start timestamp
				frame[0][0] - frame duration
				frame[1] - samples
			'''
			if len(frame) == 2:
				frames.append(frame)
				recorded += len(frame[1])
					
				

HOST = "0.0.0.0"
PORT = int(sys.argv[1])

print("Listen to %s:%d" % (HOST, PORT))

server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)

def receiver_thread_routine ():
	global server
	try:
		print("server starting")
		server.serve_forever()
	except:
		print("server serve_forever exception")
	
	
################################################################################

QWT_FRAME_LENGTH = 5000

class DataPlot(Qwt.QwtPlot):

    def __init__(self, *args):
        Qwt.QwtPlot.__init__(self, *args)

        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        # Initialize data
        self.x = range(0, QWT_FRAME_LENGTH)
        self.y = zeros(len(self.x), int)

        self.setTitle("A Moving QwtPlot Demonstration")
        self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        self.curveL = Qwt.QwtPlotCurve("Data Moving Left")
        self.curveL.attach(self)

#        self.curveL.setSymbol(Qwt.QwtSymbol(Qwt.QwtSymbol.Ellipse,
#                                        Qt.QBrush(),
#                                        Qt.QPen(Qt.Qt.yellow),
#                                        Qt.QSize(7, 7)))

        self.curveL.setPen(Qt.QPen(Qt.Qt.blue))

        mY = Qwt.QwtPlotMarker()
        mY.setLabelAlignment(Qt.Qt.AlignRight | Qt.Qt.AlignTop)
        mY.setLineStyle(Qwt.QwtPlotMarker.HLine)
        mY.setYValue(0.0)
        mY.attach(self)

        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")
    
        self.startTimer(50)
        self.phase = 0.0

    # __init__()

    def alignScales(self):
        self.canvas().setFrameStyle(Qt.QFrame.Box | Qt.QFrame.Plain)
        self.canvas().setLineWidth(1)
        for i in range(Qwt.QwtPlot.axisCnt):
            scaleWidget = self.axisWidget(i)
            if scaleWidget:
                scaleWidget.setMargin(0)
            scaleDraw = self.axisScaleDraw(i)
            if scaleDraw:
                scaleDraw.enableComponent(
                    Qwt.QwtAbstractScaleDraw.Backbone, False)

    # alignScales()

    def timerEvent(self, e):
		global frames
		
		if len(frames) > 0:
			frame = frames.pop()
			
#			print(frame)
			
			print(len(frame[1]), len(self.y))
			
			self.y = concatenate([self.y, frame[1]])
			
			self.y = self.y[-QWT_FRAME_LENGTH:]

			self.curveL.setData(self.x, self.y)

			self.replot()

    # timerEvent()

# class DataPlot



def main(args): 
	global server
	
	thread.start_new_thread(receiver_thread_routine, ())

	app = Qt.QApplication(args)
	demo = DataPlot()
	demo.resize(800, 600)
	demo.show()
	
	app.exec_()
	
	print("server close")
	server.server_close()


if __name__ == '__main__':
    main(sys.argv)














