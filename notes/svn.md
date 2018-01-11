
### branch

    svn copy trunk http://svn.example.com/myrepo/branchs/MyVer

### resolve

    svn resolve --accept theirs-full a.cpp
    svn resolve --accept base a.cpp
    svn resolve --accept working a.cpp

### http://stackoverflow.com/questions/4881129/how-do-you-see-recent-svn-log-entries

    svn log --limit 3
    svn log -r 1:HEAD
    svn log -r HEAD:1
    svn log -r 1:BASE

### http://svnbook.red-bean.com/en/1.7/svn.branchmerge.using.html
### http://blog.csdn.net/feliciafay/article/details/8962515

    svn diff -r94239
    svn diff -r94239:94127  

    svn diff --summarize -r780:HEAD

    svn st |grep '^M' |awk '{print $2}' |cpio -vpud ~/tmp/jni

### svn diff & patch

diff生成patch文件:

    svn diff > patchFile // 整个工程的变动生成patch

or:

    svn diff file > patchFile // 某个文件单独变动的patch

patch:

    patch -p0 < test.patch // -p0 选项要从当前目录查找目的文件（夹）

or

    patch -p1 < test.patch // -p1 选项要从当前目录查找目的文件，不包含patch中的最上级目录（夹）

例如两个版本以a,b开头，而a,b并不是真正有效地代码路径，则这时候需要使用"-p1"参数。

    a/src/...
    b/src/... 

svn回滚：

    svn revert FILE // 单个文件回滚
    svn revert DIR --depth=infinity // 整个目录进行递归回滚

###

    svn checkout svn://x.x.x.x/a@r123 r123

