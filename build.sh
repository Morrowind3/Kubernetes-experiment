#!/usr/bin/env bash
set -e

echo "== Generating Go protobuf/gRPC bindings =="
mkdir -p Aggregator/protobuf/press Aggregator/protobuf/fatigue

protoc --go_out=. --go_opt=module=Kubernetes2 \
       --go-grpc_out=. --go-grpc_opt=module=Kubernetes2 \
       -I. PressSimulator/press.proto FatigueWorker/fatigue.proto

echo "== Building PressSimulator =="
cmake -B PressSimulator/build -S PressSimulator -DCMAKE_BUILD_TYPE=Release
cmake --build PressSimulator/build

echo "== Building FatigueWorker =="
cmake -B FatigueWorker/build -S FatigueWorker -DCMAKE_BUILD_TYPE=Release
cmake --build FatigueWorker/build


# echo "== Building Aggregator =="
# go build -o build/aggregator ./Aggregator