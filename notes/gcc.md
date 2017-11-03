

    cpp -dM /dev/null

### https://stackoverflow.com/questions/19470873/why-does-gcc-generate-15-20-faster-code-if-i-optimize-for-size-instead-of-speed?rq=1

    g++ -O2 -falign-functions=16 -falign-loops=16

    # link time optimization
    g++ -O2 -flto add.cpp main.cpp

