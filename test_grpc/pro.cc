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
        std::uint16_t p = 50051;
        for (int i = 1; i <= n; i++) 
        {
            if (p == port) {p++;continue;}
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + std::to_string(p),
                                               grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            st.emplace_back(std::move(tmp));
            p++;
        }
      } 
      
      void leader()
      {
        AppendEntriesMessage req;
        Reply rep;
        ClientContext cont;
        req.set_term(currentTerm);
        req.set_leaderID(std::to_string(port));
        while (1) 
        {
            //这里也要并发
            sleep(0.1);
        }
      }
      
      void Heartbeat()
      {
        auto startTime = clock();      
        int timeout = rand() % 150 + 151;
        while (1) 
        {
            currentTerm++;
            if (flag.load()) 
            {
                startTime = clock(); 
                flag.store(false);
                timeout = rand() % 150 + 151;
            }
            
            if ((double)(clock() - startTime / CLOCKS_PER_SEC) * 1000 > timeout)
            {
                 int cnt = 1;
                 for (auto & stub_ : st) //这里要并发
                 {
                    RequestVoteMessage req;
                    req.set_term(currentTerm);
                    req.set_candidateID(std::to_string(port));
                    /*
                        need to set something more
                    */
                    Reply rep;
                    ClientContext cont;
                    Status status = stub_->RequestVote(&cont, req, &rep);
                    if (!status.ok()) 
                        std::cout << status.error_code() << ": " << status.error_message() << "\n";
                    std::cout << rep.response() << "\n";
                    if (rep -> ans()) cnt++;
                    if (cnt > n / 2) leader();
                 }
            }
        }
      }
      
      Status RequestVote(ServerContext* context, const RequestVoteMessage* request, Reply* reply) override
      {
        reply.set_ans(true);
        reply.set_term(currentTerm);
        if (request -> term() < currentTerm) reply.set_ans(false);
        else if (votedFor) reply.set_ans(false);
        else votedFor = 1;
        return Status::OK;
      }
      
      Status AppendEntries(ServerContext* context, const AppendEntriesMessage* request,
                           Reply* reply) override 
      {
        flag.store(true);
        if (currentTerm < request -> term()) currentTerm = request -> term(); //currentTerm 共享 !!
        reply.set_term(currentTerm);
        reply.set_ans(true);
        return Status::OK;
      }
        
      Status modify(ServerContext* context, const Vrequest* request, Vresponse* reply) override 
      {
        key_name = request -> request();
        std::string answer = std::to_string(port) + ": modify OK";
        reply->set_response(answer);
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
       int currentTerm,votedFor;
       int commitIndex,lastApplied;
       std::uint16_t port;
       std::vector<std::unique_ptr<Vergil::Stub>> st;
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
  std::thread t(&ServiceImpl::Heartbeat,&service);
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
