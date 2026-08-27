#!/usr/bin/env bash
set -e

echo "== Generating Go protobuf/gRPC bindings =="
mkdir -p Aggregator/protobuf/press Aggregator/protobuf/fatigue

protoc --go_out=. --go_opt=module=Kubernetes2 \
       --go-grpc_out=. --go-grpc_opt=module=Kubernetes2 \
       -I. PressSimulator/press.proto FatigueWorker/fatigue.proto

echo "== Building PressSimulator =="
cd PressSimulator
conan install . --build=missing
cmake --preset conan-release
cmake --build --preset conan-release
docker build . -t k8s/press
cd ../


echo "== Building FatigueWorker =="
cd FatigueWorker
conan install . --build=missing
cmake --preset conan-release
cmake --build --preset conan-release
docker build . -t k8s/fatigue

cd ../

echo "== Building Aggregator =="
cd Aggregator
go build
docker build . -t k8s/aggregator
cd ../
