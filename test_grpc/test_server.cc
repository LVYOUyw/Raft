#include <cstdio>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <thread>
#include "test_proto.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::Vrequest;
using test::Vresponse;
using test::Vergil;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class ServiceImpl final : public Vergil::Service 
{
  public:
      ServiceImpl(std::uint16_t port) : port(port) 
      {
        key_name=std::to_string(port);
        std::uint16_t p = 50051;
        for (int i = 1; i <=3 ; i++) 
        {
            if (p == port) {p++;continue;}
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + std::to_string(p),
                                               grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            st.emplace_back(std::move(tmp));
            p++;
        }
      } 
        
      Status modify(ServerContext* context, const Vrequest* request, Vresponse* reply) override 
      {
        key_name = request -> request();
        std::string answer = std::to_string(port) + ": modify OK";
        reply->set_response(answer);
        std::uint16_t p = 50051;
        for (auto & stub_ : st) 
        {
            Vrequest req;
            req.set_request(request -> request());
            Vresponse rep;
            ClientContext cont;
            Status status = stub_->smodify(&cont, req, &rep);
            if (!status.ok()) 
                std::cout << status.error_code() << ": " << status.error_message() << "\n";
            std::cout << rep.response() << "\n";
        }
        return Status::OK;
      }
      
      Status smodify(ServerContext* context, const Vrequest* request, Vresponse* reply) override 
      {
        key_name = request -> request();
        std::string answer = std::to_string(port) + ": modify OK";  
        reply -> set_response(answer);
        return Status::OK;
      }
  
  private:
       std::string key_name;
       std::uint16_t port;
       std::vector<std::unique_ptr<Vergil::Stub>> st;
};

void RunServer(std::uint16_t port) {
  std::string server_address("0.0.0.0:" + std::to_string(port));
  ServiceImpl service(port);
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) 
{
  std::thread t1(RunServer,50051);
  std::thread t2(RunServer,50052);
  std::thread t3(RunServer,50053);
  t1.join();
  t2.join();
  t3.join();
  return 0;
}
