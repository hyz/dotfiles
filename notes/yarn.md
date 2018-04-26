
https://yarnpkg.com/lang/en/docs/migrating-from-npm/

### https://github.com/yarnpkg/yarn

    curl -o- -L https://yarnpkg.com/install.sh | bash
    yarn global add create-react-app
    ls -l ~/.yarn/bin
    export PATH=$PATH:$HOME/.yarn/bin

Fast, reliable, and secure dependency management. https://yarnpkg.com

    yarn -v
    yarn run dist
    yarn run app
    yarn
    yarn run
    yarn global add create-react-native-app
    yarn --help
    yarn bin --help
    yarn add react-navigation
    yarn start
    yarn global add exp

### https://yarnpkg.com/en/docs/install#linux

    curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
    echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list

### https://yarn.bootcss.com/blog/2016/11/24/offline-mirror.html

    yarn config set yarn-offline-mirror .yarn-offline-mirror

