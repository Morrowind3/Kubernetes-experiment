cmake_minimum_required(VERSION 3.25.1)
project(PressSimulator)

set(CMAKE_CXX_STANDARD 20)


find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(prometheus-cpp REQUIRED)

set(PROTO_FILE ${CMAKE_SOURCE_DIR}/press.proto)
set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/generated)

file(MAKE_DIRECTORY ${GENERATED_DIR})

#generate messages
protobuf_generate(
    LANGUAGE cpp
    OUT_VAR PROTO_GENERATED_FILES
    PROTOS ${PROTO_FILE}
    IMPORT_DIRS ${CMAKE_SOURCE_DIR}
    PROTOC_OUT_DIR ${GENERATED_DIR}
)
#generate services
protobuf_generate(
    LANGUAGE grpc #Not a language, just ugly API design. This gets inserted in --<LANGUAGE>_out
    OUT_VAR GRPC_GENERATED_FILES
    PROTOS ${PROTO_FILE}
    IMPORT_DIRS ${CMAKE_SOURCE_DIR}
    PROTOC_OUT_DIR ${GENERATED_DIR}
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
)

add_executable(PressSimulator
    main.cpp
    ${PROTO_GENERATED_FILES}
    ${GRPC_GENERATED_FILES}
        press_data.h
        utils.h
        press_sim_service.cpp
)

target_include_directories(PressSimulator PRIVATE ${GENERATED_DIR} )
target_link_libraries(PressSimulator PRIVATE protobuf::libprotobuf gRPC::grpc++ prometheus-cpp::pull)
