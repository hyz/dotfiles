
    rg -swt java GameDefineG2 src

    cd jni
    rg -t cpp --files |grep -vE '(3rdparty|Crypto)' | ctags --c++-kinds=+px -L-

