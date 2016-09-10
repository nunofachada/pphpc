f = open('dump_agents.txt', 'r')

sheep_energy = []
wolves_energy = []

for l in f:
    if l[0] == '[':
        fields = l.split()
        en = int(fields[-1].split('=')[1])
        agt = fields[-2][-1]
        if agt == '1':
            sheep_energy.append(en)
        else:
            wolves_energy.append(en)
        

print 'Sheep:'
print '\tAvg energy:', sum(sheep_energy) / float(len(sheep_energy))
print '\tMax energy:', max(sheep_energy)
print 'Wolves:'
print '\tAvg energy:', sum(wolves_energy) / float(len(wolves_energy))
print '\tMax energy:', max(wolves_energy)

f.close()
