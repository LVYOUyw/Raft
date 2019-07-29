#include <cstdio>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <vector>
#include "server.hpp"
#include "test_proto.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::Vergil;
int n;
std::vector<Service> V;

int main(int argc, char** argv) 
{
    scanf("%d",&n);  
    for (int i=1;i<=n;i++) 
    {
        Service tmp;
        tmp.Start(50051 + i - 1);
        V.emplace_back(std::move(tmp));
    }
   // for (int i=0;i<n;i++) T[i].join();
    return 0;
}
