
### https://github.com/golang/go/wiki/Ubuntu

    sudo add-apt-repository ppa:gophers/archive
    sudo apt update
    sudo apt-get install golang-1.9-go

### https://askubuntu.com/questions/720260/updating-golang-on-ubuntu

Step 1: Remove the existing golang

    sudo apt-get purge golang*

Step 2: Download the latest version from the official site. Click Here

Step 3: Extract it in /usr/local using the following command

    tar -C /usr/local -xzf go$VERSION.$OS-$ARCH.tar.gz

Step 4: Create .go directory in home. (It is easy to install the necessary packages without admin privillege)

    mkdir ~/.go

Step 5: Set up the following environment variables

    export GOROOT=/usr/local/go
    export GOPATH=~/.go
    export PATH=$PATH:$GOROOT/bin:$GOPATH/bin

Step 6: Update the go command

    sudo update-alternatives --install "/usr/bin/go" "go" "/usr/local/go/bin/go" 0
    sudo update-alternatives --set go /usr/local/go/bin/go

Step 7: Test the golang version

    go version

