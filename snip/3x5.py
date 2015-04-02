
for i in range(150): # print('_',sep='',end='')
    if i % 3==0 and i % 5==0:
        print('+',sep='',end='')
    elif i % 3==0:
        print('3',sep='',end='')
    elif i % 5==0:
        print('5',sep='',end='')
    else:
        print('-',sep='',end='')

