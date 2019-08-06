#include <cstdio>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include "server.hpp"
#include "External.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::Vergil;
using external::External;
int n;
std::vector<boost::thread> V;

void Run(int port)
{
    Service tmp1;
    ExternalService tmp2;
    tmp1.Start(port);
    tmp2.Start(port+5);
}

int main(int argc, char** argv)
{

    scanf("%d",&n);
/*    for (int i=0;i<1;i++)
    {
        boost::thread th(boost::bind(Run,n));
        V.emplace_back(std::move(th));
    }
    for (int i=0;i<1;i++)
        if (V[i].joinable()) V[i].join();*/
    Run(n);
    return 0;
}
