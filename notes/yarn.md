
yarn run build --mode production

### https://yarnpkg.com/lang/en/docs/migrating-from-npm/

### https://github.com/yarnpkg/yarn

    yarn run serve --host 0.0.0.0

    curl -o- -L https://yarnpkg.com/install.sh | bash
    yarn global add create-react-app

    ls -l ~/.yarn/bin
    export PATH=$PATH:$HOME/.yarn/bin

Fast, reliable, and secure dependency management. https://yarnpkg.com

    yarn versions
    yarn info webpack

    yarn global add create-react-native-app
    yarn add @swc/core @swc/cli --dev
    yarn global add exp

### https://yarnpkg.com/en/docs/install#linux

    curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
    echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list

### https://yarn.bootcss.com/blog/2016/11/24/offline-mirror.html

    yarn config set yarn-offline-mirror .yarn-offline-mirror


yarn outdated
yarn upgrade --latest
yarn bin
yarn list
yarn run env

https://classic.yarnpkg.com/en/docs/cli/run/

yarn config get registry
yarn config set registry https://registry.yarnpkg.com
yarn config set registry https://registry.npm.taobao.org/

npm config set registry https://registry.npm.taobao.org/
npm config get registry


yarn ts-node-dev src/index.ts
yarn webpack --mode=development
yarn ts-node-dev src/main.ts
yarn info
