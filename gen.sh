#bash
/usr/local/bin/protoc --proto_path=src/protos --cpp_out=src/generated src/protos/*.proto
/usr/local/bin/protoc --proto_path=src/protos --grpc_out=src/generated --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin src/protos/*.proto