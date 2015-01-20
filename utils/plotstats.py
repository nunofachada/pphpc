#!/usr/bin/env python

from pylab import *

x=loadtxt('stats.txt')
plot(x[:,0], 'b', x[:,1], 'r', x[:,2]/4, 'g')
legend(['Sheep','Wolves','Grass/4'])
grid(True)
show()
