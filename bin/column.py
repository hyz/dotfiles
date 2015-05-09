
# calculate the lenghthiest term per colum
MaxRow = [max(map(len, col)) for col in zip(*Checks)]

Checks = [
['a','b','ccc'],
['1','22222','3'],
['xx','y','z']
]

# the first line of checks is a header which requires
# a row of equal signs to follow
hdr = "  ".join((' ' + val.rjust(Max) + ' |' for val, Max in zip(Checks[0], MaxRow)))[:-1]
print hdr
print '=' * len(hdr)

# delete the header and output the remaining table
del Checks[0]
# finish table
for row in Checks:
	tbl = "  ".join((' ' + val.rjust(Max) + ' |' for val, Max in zip(row, MaxRow)))[:-1]
	print tbl

for row in Checks:
	print "  ".join((val.rjust(Max) for val, Max in zip(row, MaxRow)))



