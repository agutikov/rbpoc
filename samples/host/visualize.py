#!/usr/bin/python

from pprint import pprint
from sqlitedict import SqliteDict
import operator
import collections
import time

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

mydict = SqliteDict('./my_db.sqlite', autocommit=False)


deleted = 0

fig = plt.figure()
ax = fig.add_subplot(111)
line, = ax.plot([], [])


def update(data):
	if len(data[0]) == len(data[1]) == 2000:
		line.set_ydata(data[1])
		line.set_xdata(data[0])
		min_y = min(data[1])
		max_y = max(data[1])
		min_x = min(data[0])
		max_x = max(data[0])
		print(min_y, max_y, min_x, max_x)
		ax.set_ylim(min_y - 20, max_y + 20)
		ax.set_xlim(min_x - 0.05, max_x + 0.05)
	else:
		print("wrong data length")
	return line,


def data_gen():
	global deleted
	global mydict
	
	while True:
		print("len: ", len(mydict))
		
		values = []
		rel_timestamps = []
			
		if len(mydict) >= 2000:

			timestamps = sorted(mydict)[:2000]
			
			for k in timestamps:
				values.append(int(mydict[k]))

			for k in mydict:
				del mydict[k]
			mydict.commit()
			
			t_start = float(timestamps[0])
			for t in timestamps:
				rel_timestamps.append(float(t) - t_start)

		else:
			time.sleep(0.1)

		yield [rel_timestamps, values]


ani = animation.FuncAnimation(fig, update, data_gen, blit=False, repeat=True, interval=0)


try:
	plt.show()
except KeyboardInterrupt:
	pass

mydict.close()

