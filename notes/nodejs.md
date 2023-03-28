
    eval "$(fnm env --use-on-cd)"

### https://www.digitalocean.com/community/tutorials/how-to-install-node-js-on-debian-8

    sudo apt-get install build-essential cmake

    curl -sL https://deb.nodesource.com/setup_9.x -o nodesource_setup.sh

### webpack-dev-server

    npm run serve -- --host 0.0.0.0

> @ serve /home/wood/rs/wasm-bindgen/examples/hello_world
> webpack-dev-server "--host" "0.0.0.0"

wds｣: Project is running at http://0.0.0.0:8080/


　　yarn最常用最基础的命令和npm对比

　　npm init /  yarn init  初始化

　　mkdir 文件名 /  md  文件名

　　npm install / yarn或yarn install  安装依赖

　  npm install package -g / yarn global add package 全局安装某个依赖

　　npm install package --save-dev  /   yarn add package --dev  安装某个依赖

　　npm uninstall package --save-dev /  yarn remove package --dev  卸载某个依赖

　　npm run dev 或 npm start  /  yarn run start 或 yarn start  运行

## https://www.cnblogs.com/tu-0718/p/12571143.html

npm i --save-dev @types/node

