
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


### https://stackoverflow.com/questions/8540485/how-do-i-split-a-string-in-make

split host:port

    # Retrieves a host part of the given string (without port).
    # Param:
    #   1. String to parse in form 'host[:port]'.
    host = $(firstword $(subst :, ,$1))

    # Returns a port (if any).
    # If there is no port part in the string, returns the second argument
    # (if specified).
    # Param:
    #   1. String to parse in form 'host[:port]'.
    #   2. (optional) Fallback value.
    port = $(or $(word 2,$(subst :, ,$1)),$(value 2))

    Usage:

    $(call host,foo.example.com) # foo.example.com
    $(call port,foo.example.com,80) # 80

    $(call host,ssl.example.com:443) # ssl.example.com
    $(call port,ssl.example.com:443,80) # 443

### https://stackoverflow.com/questions/32176074/function-patsubst-in-makefile

For PATTERN = X:
    ...
For PATTERN = X%:
    ...

