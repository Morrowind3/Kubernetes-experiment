#!/usr/bin/env bash
set -e

echo "== Generating Go protobuf/gRPC bindings =="
mkdir -p Aggregator/protobuf/press Aggregator/protobuf/fatigue

protoc --go_out=. --go_opt=module=Kubernetes2 \
       --go-grpc_out=. --go-grpc_opt=module=Kubernetes2 \
       -I. PressSimulator/press.proto FatigueWorker/fatigue.proto

echo "== Building PressSimulator =="
cmake -B PressSimulator/cmake-build-release -S PressSimulator -DCMAKE_BUILD_TYPE=Release
cmake --build PressSimulator/cmake-build-release
docker build PressSimulator/ -t k8s/press

echo "== Building FatigueWorker =="
cmake -B FatigueWorker/cmake-build-release -S FatigueWorker -DCMAKE_BUILD_TYPE=Release
cmake --build FatigueWorker/cmake-build-release
docker build FatigueWorker/ -t k8s/fatigue

echo "== Building Aggregator =="
cd Aggregator
go build
docker build . -t k8s/aggregator
cd ../
