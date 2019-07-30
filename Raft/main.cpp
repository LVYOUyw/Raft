#include <cstdio>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <vector>
#include <boost/thread/thread.hpp>
#include "server.hpp"
#include "test_proto.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::Vergil;
int n;
std::vector<boost::thread> V;

void Run(int port) 
{
    Service tmp;
    tmp.Start(port);
}

int main(int argc, char** argv) 
{
    scanf("%d",&n);  
    for (int i=0;i<n;i++) 
    {
        boost::thread th(boost::bind(Run,i+50051));
        V.emplace_back(std::move(th));
    }
    for (int i=0;i<n;i++) V[i].join();
    return 0;
}
