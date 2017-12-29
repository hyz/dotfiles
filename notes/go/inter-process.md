
### https://gobyexample.com/spawning-processes
### https://gobyexample.com/execing-processes

    exec.Command
    exec.LookPath
    syscall.Exec

    signal.Notify

### https://github.com/golang/go/issues/227

    import (
     "syscall"
     "os"
    )
    func daemon (nochdir, noclose int) int {
        var ret uintptr
        var err uintptr
        ret,_,err = syscall.Syscall(syscall.SYS_FORK, 0, 0, 0)
        if err != 0 { return -1 }
        switch ret {
            case 0: break
            default: os.Exit(0)
        }
        if syscall.Setsid () == -1 { return -1 }
        if nochdir == 0 { os.Chdir("/") }
        if noclose == 0 {
            if f, e := os.Open ("/dev/null", os.O_RDWR, 0) ; e == nil {
                fd := f.Fd ()
                syscall.Dup2 (fd, os.Stdin.Fd ())
                syscall.Dup2 (fd, os.Stdout.Fd ())
                syscall.Dup2 (fd, os.Stderr.Fd ())
            }
        }
        return 0
    }

### http://colobu.com/2015/10/09/Linux-Signals/

https://github.com/fvbock/endless
https://github.com/facebookgo/grace

