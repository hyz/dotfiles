
### How can I see the exact commands?

    make VERBOSE=1
    make V=1

OR

    cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON .
    make

    make DESTDIR=dist install

https://stackoverflow.com/questions/5820303/how-do-i-force-make-gcc-to-show-me-the-commands
https://stackoverflow.com/questions/2670121/using-cmake-with-gnu-make-how-can-i-see-the-exact-commands?noredirect=1&lq=1

### http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

Auto-Dependency Generation

### http://stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile

In terms of Make, a phony target is simply a target that is always out-of-date, so whenever you ask make <phony_target>, it will run, independent from the state of the file system

###

http://serve.3ezy.com/stackoverflow.com/questions/2481269/how-to-make-a-simple-c-makefile/
http://serve.3ezy.com/stackoverflow.com/questions/448910/makefile-variable-assignment?rq=1
http://serve.3ezy.com/stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile?rq=1

