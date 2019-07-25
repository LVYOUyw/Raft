#ifndef PPCA_RAFT_SERVER
#define PPCA_RAFT_SERVER

#include <thread>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
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

class ServiceImpl final : public Vergil::Service 
{
    public:
        Status RequestVote(ServerContext* context, const RequestVoteMessage* request, 
                           Reply* reply)     override      
        {
            
        }
        
        Status AppendEntries(ServerContext* context, const AppendEntriesMessage* request, 
                           Reply* reply)     override      
        {
        
        }
    private:
         std::function<RPCReply(const AppendEntiresRPC &)> append;
         std::function<RPCReply(const RequestVoteRPC &)> vote;
};


class Service
{
    public:    
        
        void Start(uint16_t port) 
        {
            std::string server_address("0.0.0.0:" + std::to_string(port));
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            serv=builder.BuildAndStart();
            Port = port;
            runningThread = std::thread([this] { serv -> Wait(); });
        }
        
        void Shutdown() 
        {
            if (serv) 
                serv -> Shutdown();
            runningThread.join();
        }
          
  
    private:
        ServiceImpl service;
        std::unique_ptr<Server> serv;
        std::thread runningThread;
        uint16_t Port;
       
};

#endif 

