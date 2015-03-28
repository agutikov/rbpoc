#!/usr/bin/python

from pprint import pprint
from sqlitedict import SqliteDict
import operator
import collections
import time

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

mydict = SqliteDict('./my_db.sqlite', autocommit=True)


deleted = 0

fig, ax = plt.subplots()
line, = ax.plot(np.random.rand(1000))
ax.set_ylim(-50, 1200)


def update(data):
	if len(data) == 1000:
		line.set_ydata(data)
	return line,


def data_gen():
	global deleted
	global mydict
	
	while True:
		print("len: ", len(mydict))
		
		values = []
			
		if len(mydict) >= 1000:
			
			sorted_keys = sorted(mydict)
						
			timestamps = sorted_keys[:1000]
			
			if len(mydict) >= 1200:
				to_del = 100
			else:
				to_del = 0
				
			for k in timestamps:
				values.append(mydict[k])
				if to_del > 0:
					del mydict[k]
					deleted += 1
					to_del -= 1

			print("deleted:", deleted)
		else:
			time.sleep(0.1)

		yield values


ani = animation.FuncAnimation(fig, update, data_gen, interval=100)


try:
	plt.show()
except KeyboardInterrupt:
	pass

mydict.close()

