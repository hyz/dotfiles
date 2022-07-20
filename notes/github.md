
    GITHUB_TOKEN=...  hub create hyzex1

    git clone -o seed -b main --single-branch git://github.com/kriasoft/react-starter-kit exa1

    git config --global http.sslVerify false
    git config --global --add remote.origin.proxy ""
    git config --global --unset-all remote.origin.proxy

### git.oschina.net

    https://gitee.com/projects/import/url

### https://github.com/hunshcn/gh-proxy

    https://hunsh.net/archives/23/

    git clone https://gh.api.99988866.xyz/https://github.com/unicode-org/icu4x

    git clone --recursive -j8 https://ghproxy.com/https://github.com/unicode-org/icu4x

    https://octoverse.github.com/

more proxy

    https://ghproxy.com/https://github.com/hyz/...
    https://download.fastgit.org/hyz/...

### awesome

https://github.com/kgryte/awesome-peer-to-peer
https://github.com/aturl/awesome-anti-gfw/blob/master/WireGuard_VPN_Tunnel.md


### 断点续传 https://blog.csdn.net/sayai/article/details/81452496

    git init   //找个空文件夹，初始化一个repo

    git fetch git://github.com/xmoeproject/KrkrExtract   // 用http也会断，只有用git才行

    git checkout FETCH_HEAD  // 成功提取也就ok了。

    中间可能碰到权限问题，需要去git上增加一个ssh的key

    Please make sure you have the correct access rights and the repository exists.

    命令行执行以下语句

    git config --global user.name 【你的登录用户】

    git config --global user.email 【你的注册邮箱】

    ssh-keygen -t rsa -C "your@email.com"   // 等待提示，然后输入yes

    按照提示去目标的.ssh文件夹下找到id_rsa.pub文件，用记事本打开，拷贝全部内容。

    打开https://github.com/，登陆你的账户，进入设置，然后是SSH and GPG keys。

    New SSH Key 把刚刚拷贝的内容贴进去，title随便起。

    回到命令行中执行：ssh -T git@github.com

    等待提示出现后，输入yes。

    然后可以继续刚刚命令行的步骤了。




### github.com.cnpmjs.org

    https://github.com/sminez/roc 
    https://github.com.cnpmjs.org/sminez/roc/

### hub init & create

    cd Some-Exist-Dir/

    hub init
    hub add .
    hub commit -m 'initial ...'
    GITHUB_TOKEN=...  hub create # Some-Exist-Dir
    GITHUB_TOKEN=...  hub create my-example
    hub push -u origin HEAD # hub push --set-upstream origin main

