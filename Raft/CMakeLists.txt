cmake_minimum_required(VERSION 3.5.0)
project(TEST_RAFT)

set(CMAKE_CXX_FLAGS "-std=c++14 -g")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
FIND_PACKAGE(Boost 1.58 COMPONENTS program_options thread REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

link_libraries(grpc++ protobuf pthread)

add_executable(server main.cpp heartbeat.hpp server.hpp External.hpp test_proto.grpc.pb.cc test_proto.pb.cc external.grpc.pb.cc external.pb.cc)
TARGET_LINK_LIBRARIES(server LINK_PUBLIC ${Boost_LIBRARIES})
