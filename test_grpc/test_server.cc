#include <cstdio>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <atomic>
#include <vector>
#include <unistd.h>
#include <ctime>
#include "test_proto.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::AppendEntriesMessage;
using test::RequestVoteMessage;
using test::Reply;
using test::Vrequest;
using test::Vresponse;
using test::Vergil;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
std::vector<std::thread> Threads;
int n,o=50051;

class ServiceImpl final : public Vergil::Service 
{
  public:
      ServiceImpl(std::uint16_t port) : port(port) 
      {
        flag.store(false);
        currentTerm = votedFor = 0;
        key_name=std::to_string(port);
      } 
      
      void modifytogether(uint16_t p) 
      {
        std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + std::to_string(p),
                                               grpc::InsecureChannelCredentials());
        std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
        Vrequest req;
        //req.set_request(s);
        Vresponse rep;
        ClientContext cont;
        Status status = tmp->smodify(&cont, req, &rep);
        if (!status.ok()) 
            std::cout << status.error_code() << ": " << status.error_message() << "\n";
        std::cout << rep.response() << "\n";
      }
        
      Status modify(ServerContext* context, const Vrequest* request, Vresponse* reply) override 
      {
        key_name = request -> request();
        std::string answer = std::to_string(port) + ": modify OK";
        reply->set_response(answer);
        uint16_t  p = 50051;
        std::string tt = request -> request();
        std::vector<std::thread> Tds;
        for (int i = 1; i <= n; i++) 
        {
            printf("%d\n",i);
            if (p == port) {p++;continue;}
            std::thread t(&ServiceImpl::modifytogether,this,p);
            Tds.push_back(std::move(t));
            p++;
        }
        for (int i = 0; i < n - 1; i++)  Tds[i].join();
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
       int currentTerm,votedFor;
       int commitIndex,lastApplied;
       std::uint16_t port;
       std::vector<int> nextIndex;
       std::vector<int> matchIndex;
       std::atomic<bool> flag;
  
};

void RunServer(std::uint16_t port) 
{
  std::string server_address("0.0.0.0:" + std::to_string(port));
  ServiceImpl service(port);
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
 // std::thread t(&ServiceImpl::Heartbeat,&service);
  server->Wait();
}

int main(int argc, char** argv) 
{
  scanf("%d",&n);
  for (int i=1;i<=n;i++) 
  {
    std::thread t(RunServer,o);
    Threads.push_back(std::move(t));
    o++;
  }
  for (int i=0;i<n;i++) Threads[i].join();
  return 0;
}
