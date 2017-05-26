
### https://stackoverflow.com/questions/36808565/using-libclang-to-parse-in-c-in-python

import clang.cindex

s = '''
int fac(int n) {
    return (n>1) ? n*fac(n-1) : 1;
}
'''

idx = clang.cindex.Index.create()
tu = idx.parse('tmp.cpp', args=['-std=c++11'],  
                unsaved_files=[('tmp.cpp', s)],  options=0)
for t in tu.get_tokens(extent=tu.cursor.extent):
    print(t.kind)

for x in dir(t):
    print(x,getattr(t,x))

