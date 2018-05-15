
### ubuntu-make

    add-apt-repository ppa:ubuntu-desktop/ubuntu-make
    apt-get update
    apt-get install ubuntu-make

### visual studio code

    umake web visual-studio-code
    umake web visual-studio-code --remove

### IntelliJ IDEA 

    apt-add-repository ppa:mmk2410/intellij-idea

### atom, https://linux.cn/article-3663-1.html
### atom, https://studygolang.com/articles/5943

    add-apt-repository ppa:webupd8team/atom
    apt-get update
    apt-get install atom

    apt-get remove atom
    add-apt-repository --remove ppa:webupd8team/atom

### java 8

    add-apt-repository ppa:webupd8team/java
    apt-get update
    apt-get install oracle-java8-installer
    apt-get install oracle-java8-set-default

### golang, https://www.jianshu.com/p/ff3bc1b7abdd
### golang, https://www.cnblogs.com/zsy/archive/2016/02/28/5223957.html

go-plus
vim-mode-plus

    go get -u -v github.com/tpng/gopkgs
    go get -u -v github.com/nsf/gocode
    https_proxy=192.168.2.115:8118 go get -u -v sourcegraph.com/sqs/goreturns

    go get -u -v github.com/rogpeppe/godef
    go get -u -v github.com/golang/lint/golint

    go get -u -v github.com/lukehoban/go-find-references
    go get -u -v golang.org/x/tools/cmd/gorename
    go get -u -v github.com/newhook/go-symbols
    go get -u -v github.com/lukehoban/go-outline

    go get -u golang.org/x/tools/cmd/...
