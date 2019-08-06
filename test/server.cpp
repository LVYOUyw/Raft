#include <cstdio>
#include <iostream>
#include <string>
#include <cstdlib>
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

int main(int argc, char **argv) 
{
	int id = atoi(argv[1]);
	std::cerr<<"serverID: "<<id<<"\n";
	Service tmp1;
	ExternalService tmp2;
	tmp1.Start(id+50051);
	tmp2.Start(id+50056);
	while (1);
	return 0;
}
