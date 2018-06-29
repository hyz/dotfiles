
    :help cedit

    :set makeprg=/opt/bin/g++\ -std=c++11\ -I/opt/boost\ %\ ~/lib/libboost_system.a\ ~/lib/libboost_serialization.a

    :g/:10014..devtok/ .w >> user-10014.txt
    :g/"cmd":\(99\|211\|111\|112\),/ .w >>10014-99-211.txt
    :g/^\s*LOG.*\\n/s/\\n//

    $ vim +"echo &runtimepath" +q
    $ proxychains vim +PlugInstall +q

### http://vim.wikia.com/wiki/Power_of_g

### http://stackoverflow.com/questions/4789811/how-do-i-repeat-any-command-in-vim-like-c-x-z-in-emacs

    @:
    q:k
    :help repeating

    :normal A^I^I^[J
    :normal \t\t\Ctrl-v<Esc>

    :.!xargs rm -f

### ubuntu

    apt-get install vim-scripts

### http://askubuntu.com/questions/216818/how-do-i-use-plugins-in-the-vim-scripts-package

    vam ## vim-addon-manager
    vam install taglist

### https://github.com/VundleVim/Vundle.vim

    git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim

### YouCompleteMe

https://github.com/Valloric/YouCompleteMe/wiki/Building-Vim-from-source
https://github.com/Valloric/YouCompleteMe

    sudo checkinstall
    dpkg -r vim
    ...

    cd ~/.vim/bundle/YouCompleteMe
    ./install.py --system-boost --go-completer --rust-completer --js-completer
    ./install.py --rust-completer --system-boost --clang-completer --system-libclang
    ./install.py --go-completer --js-completer

https://github.com/Valloric/YouCompleteMe/issues/538

    ./install.py --clang-completer --system-libclang


https://aur.archlinux.org/packages/vim-youcompleteme-core-git/
http://howiefh.github.io/2015/05/22/vim-install-youcompleteme-plugin/?utm_source=tuicool&utm_medium=referral
http://www.cnblogs.com/linux-sir/p/4676647.html

### http://stackoverflow.com/questions/71323/how-to-replace-a-character-by-a-newline-in-vim?rq=1

-1
    echo bar > test
    vim test '+s/b/\n/' '+s/a/\r/' +wq
    vim a.txt '+set fileformat=unix' +wq

-2
    :set magic
    Control-v ...

### 

    set makeprg=g++\ -g\ -pthread\ -std=c++0x\ -I/BOOST_ROOT\ -I.\ %\ -lrt


### golang

    https://github.com/fatih/vim-go  ~/.vim/bundle/vim-go
    mkdir -p ~/.vim/autoload ~/.vim/bundle && curl -LSso ~/.vim/autoload/pathogen.vim https://tpo.pe/pathogen.vim
    https://github.com/nsf/gocode

### http://coderoncode.com/tools/2017/04/16/vim-the-perfect-ide.html

### http://www.commandlinefu.com/commands/view/1204/save-a-file-you-edited-in-vim-without-the-needed-permissions

https://stackoverflow.com/questions/2600783/how-does-the-vim-write-with-sudo-trick-work

    :w !sudo tee %
    :%!sudo tee %

