
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

yarn info babel-loader |rg '(version|latest)'


## Yarn

    yarn config set registry https://registry.npm.taobao.org --global
    yarn config set disturl https://npm.taobao.org/dist --global
    yarn config set sass_binary_site https://npm.taobao.org/mirrors/node-sass --global
    yarn config set electron_mirror https://npm.taobao.org/mirrors/electron/ --global
    yarn config set puppeteer_download_host https://npm.taobao.org/mirrors --global
    yarn config set chromedriver_cdnurl https://npm.taobao.org/mirrors/chromedriver --global
    yarn config set operadriver_cdnurl https://npm.taobao.org/mirrors/operadriver --global
    yarn config set phantomjs_cdnurl https://npm.taobao.org/mirrors/phantomjs --global
    yarn config set selenium_cdnurl https://npm.taobao.org/mirrors/selenium --global
    yarn config set node_inspector_cdnurl https://npm.taobao.org/mirrors/node-inspector --global

## Npm
￼
    npm set registry https://registry.npm.taobao.org && \
    npm set disturl https://npm.taobao.org/dist && \
    npm set sass_binary_site https://npm.taobao.org/mirrors/node-sass && \
    npm set electron_mirror https://npm.taobao.org/mirrors/electron && \
    npm set puppeteer_download_host https://npm.taobao.org/mirrors && \
    npm set chromedriver_cdnurl https://npm.taobao.org/mirrors/chromedriver && \
    npm set operadriver_cdnurl https://npm.taobao.org/mirrors/operadriver && \
    npm set phantomjs_cdnurl https://npm.taobao.org/mirrors/phantomjs && \
    npm set selenium_cdnurl https://npm.taobao.org/mirrors/selenium && \
    npm set node_inspector_cdnurl https://npm.taobao.org/mirrors/node-inspector && \
    npm cache clean --force

