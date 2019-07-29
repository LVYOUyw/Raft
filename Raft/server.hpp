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
    
    RPCReply(bool Ans, int Term) : ans(Ans), term(Term) {}
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
        template <class Func> 
        void initappend(Func &&f) 
        {
            append = std::forward<Func>(f);
        }
        
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
        
        Status AppendEntries(ServerContext* context, const AppendEntriesMessage* request, 
                           Reply* reply)     override      
        {
            
            return Status::OK;
        }
    private:
         std::function<RPCReply(const AppendEntiresRPC &)> append;
         std::function<RPCReply(const RequestVoteRPC &)> vote;
};
         
class Service
{
    public:    
    
        void sendElection(uint16_t tt) 
        {
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + 
                                                   std::to_string(50051+tt),
                                                   grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            RequestVoteMessage Req;
            Reply Rep;
            Req.set_term(currentTerm);
            Req.set_candidateid(std::to_string(port));
            ClientContext cont;
            tmp -> RequestVote
        }
    
        bool Election()
        {
            currentTerm++;
            voteCnt = 1;
            votedFor = std::to_string(port);
            vector<boost::thread> V;
            for (int i=0;i<5;i++)
            {
                if (50051+i==port) continue;                
                boost::function0<void> f = boost::bind(&Service::sendElection, this, i);
                boost::thread th(f);
                V.emplace_back(th);
            }
            for (int i=0;i<4;i++)
                if (V[i].joinable()) V[i].join();
        }
        
        bool Alive()
        {
            
        }
        
        RPCReply append(const AppendEntiresRPC &message)  
        {
            
        }
        
        RPCReply vote(const RequestVoteRPC &message)
        {
            RPCReply reply(1,currentTerm);
            if (currentTerm > term) reply.ans = 0;
            if (votedFor == "") votedFor = message.candidateid;
            return reply;
        }
        
        void Start(uint16_t port) 
        {
            service.initappend(std::bind(&Service::append, this, std::placeholders::_1));
            service.initvote(std::bind(&Service::vote, this, std::placeholders::_1));
            Control.initElection(std::bind(&Service::Election, this));  
            Control.initAlive(std::bind(&Service::Alive, this));
            std::string server_address("0.0.0.0:" + std::to_string(port));
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            serv=builder.BuildAndStart();
            Port = port;
            runningThread = std::thread([this] { serv -> Wait(); });
            Control.startfollower();
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
        int currentTerm;
        std::string votedFor = 0;
        uint16_t Port;
        heartbeat Control;
        int voteCnt;
};

#endif 
