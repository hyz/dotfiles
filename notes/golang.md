
    export GOPATH=$HOME/gocode
    export PATH=$PATH:$HOME/go/bin

    go build -ldflags "-s -w"

### https://github.com/golang/go/wiki

### http://stackoverflow.com/questions/25927660/golang-get-current-scope-of-function-name

    func trace() {
        pc := make([]uintptr, 10)  // at least 1 entry needed
        runtime.Callers(2, pc)
        f := runtime.FuncForPC(pc[0])
        file, line := f.FileLine(pc[0])
        fmt.Printf("%s:%d %s\n", file, line, f.Name())
    }

### links

* http://stackoverflow.com/questions/15049903/how-to-use-custom-packages-in-golang
* https://golang.org/doc/code.html
* http://code.google.com/p/go-wiki/wiki/GithubCodeLayout
* http://www.crifan.com/go_language_process_http_cookie/

### https://github.com/golang/go/wiki/Switch

* Not just integers
* Missing expression
* Break : Go's switch statements break implicitly, but break is still useful:
* Fall through
* Multiple cases
* Type switch
* Noop case

### libraries

* https://github.com/faiface/pixel

### https://stackoverflow.com/questions/10383299/how-do-i-configure-go-to-use-a-proxy

    https_proxy=127.0.0.1:7788 http_proxy=127.0.0.1:7788 go get -v -u ...
