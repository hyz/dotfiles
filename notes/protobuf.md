
    cmake -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../release ..

    protoc --js_out=import_style=commonjs,binary:js statis.proto

## https://github.com/golang/protobuf

    go get -u github.com/golang/protobuf/protoc-gen-go


