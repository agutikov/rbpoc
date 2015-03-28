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

fig, ax = plt.subplots()
line, = ax.plot(np.random.rand(2000))
ax.set_ylim(-50, 1200)


def update(data):
	if len(data) == 2000:
		line.set_ydata(data)
	else:
		print("wrong data length")
	return line,


def data_gen():
	global deleted
	global mydict
	
	while True:
		print("len: ", len(mydict))
		
		values = []
			
		if len(mydict) >= 2000:
			
			sorted_keys = sorted(mydict)
						
			timestamps = sorted_keys[:2000]

#			if len(mydict) >= 3500:
#				to_del = 1500
#			else:
#				to_del = 0

			for k in timestamps:
				values.append(mydict[k])

#				if to_del > 0:
#					del mydict[k]
#					deleted += 1
#					to_del -= 1

			for k in mydict:
				del mydict[k]
			mydict.commit()

#			print("deleted:", deleted)
		else:
			time.sleep(0.1)

		yield values


ani = animation.FuncAnimation(fig, update, data_gen, interval=0)


try:
	plt.show()
except KeyboardInterrupt:
	pass

mydict.close()

