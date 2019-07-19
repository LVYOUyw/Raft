#include <cstdio>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include "test_proto.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::Vrequest;
using test::Vresponse;
using test::Vergil;

class ServiceImpl final : public Vergil::Service 
{
  Status testhello(ServerContext* context, const Vrequest* request, Vresponse* reply) override 
  {
    std::string prefix("Hello ");
    reply->set_response(prefix + request->request());
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ServiceImpl service;
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) 
{
  RunServer();
  return 0;
}
