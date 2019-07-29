#ifndef PPCA_RAFT_SERVER
#define PPCA_RAFT_SERVER

#include <bits/stdc++.h>  
#include <grpcpp/grpcpp.h>
#include "test_proto.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::AppendEntriesMessage;
using test::RequestVoteMessage;
using test::Reply;
using test::Vergil;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

struct RPCReply
{
    bool ans;
    int term;
};
struct AppendEntiresRPC
{

};
struct RequestVoteRPC
{
    int term;
    std::string candidateid;
    
    RequestVoteRPC(int Term, std::string id) : term(Term), candidateid(id) {}
};

class ServiceImpl final : public Vergil::Service 
{
    public:
        /*template <class Func> 
        void initappend(Func &&f) 
        {
            append = std::forward<Func>(f);
        }*/
        
        template <class Func> 
        void initvote(Func &&f) 
        {
            vote = std::forward<Func>(f);
        }
        
        Status RequestVote(ServerContext* context, const RequestVoteMessage* request, 
                           Reply* reply)     override      
        {
            RPCReply rep = vote(RequestVoteRPC(request -> term(), request -> candidateid()));
            reply -> set_ans(rep.ans);
            reply -> set_term(rep.term);
            return Status::OK;
        }
        
        /*Status AppendEntries(ServerContext* context, const AppendEntriesMessage* request, 
                           Reply* reply)     override      
        {
            return Status::OK;
        }*/
    private:
         std::function<RPCReply(const AppendEntiresRPC &)> append;
         std::function<RPCReply(const RequestVoteRPC &)> vote;
};
         
class Service
{
    public:    
        
        /*RPCReply append(const AppendEntiresRPC &) 
        {

        }*/ 
        
        RPCReply vote(const RequestVoteRPC &)
        {
        
        }
        
        void Start(uint16_t port) 
        {
            service.initappend(std::bind(&Service::append, this, std::placeholders::_1));
            service.initvote(std::bind(&Service::vote, this, std::placeholders::_1));
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
