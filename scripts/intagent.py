for i in range(5,30):
	print i, 'bits for cell index,', 31-i, 'bits for energy:'
	print '    max side:', 2.0**(i/2.0), ' | max energy:', 2**(31-i), '\n'
