#ifndef PPCA_RAFT_SERVER
#define PPCA_RAFT_SERVER

#include <bits/stdc++.h>  
#include <grpcpp/grpcpp.h>
#include "test_proto.grpc.pb.h"
#include <boost/thread/thread.hpp>
#include "heartbeat.hpp"
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
    
    RPCReply(bool Ans = 0, int Term = 0) : ans(Ans), term(Term) {}
};
struct AppendEntiresRPC
{
    int term;
    std::string leaderid;
    
    AppendEntiresRPC(int Term = 0, std::string id = "") : term(Term), leaderid(id) {}
};
struct RequestVoteRPC
{
    int term;
    std::string candidateid;
    
    RequestVoteRPC(int Term = 0, std::string id = "") : term(Term), candidateid(id) {}
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
            append(AppendEntiresRPC(request -> term(), request -> leaderid()));
            return Status::OK;
        }
    private:
         std::function<RPCReply(const AppendEntiresRPC &)> append;
         std::function<RPCReply(const RequestVoteRPC &)> vote;
};
         
class Service
{
    public:    
    
        //Service() {puts("WWW");}
    
        void sendElection(uint16_t tt) 
        {
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + 
                                                   std::to_string(50051+tt),
                                                   grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            RequestVoteMessage Req;
            Reply Rep;
            Req.set_term(currentTerm);
            Req.set_candidateid(std::to_string(Port));
            ClientContext cont;
            tmp -> RequestVote(&cont, Req, &Rep);
            if (Rep.ans()) voteCnt++;
        }
        
        void sendAlive(uint16_t tt) 
        {
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + 
                                                   std::to_string(50051+tt),
                                                   grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            AppendEntriesMessage Req;
            Reply Rep;
            Req.set_term(currentTerm);
            Req.set_leaderid(std::to_string(Port));
            ClientContext cont;
            tmp -> AppendEntries(&cont, Req, &Rep);
        }
    
        bool Election()
        {
            currentTerm++;
            voteCnt = 1;
            votedFor = std::to_string(Port);
            std::vector<boost::thread> V;
            for (int i=0;i<5;i++)
            {
                if (50051+i==Port) continue;                
                //boost::function0<void> f = boost::bind(&Service::sendElection, this, i);
                boost::thread th(boost::bind(&Service::sendElection, this, i));
                V.emplace_back(std::move(th));
            }
            for (int i=0;i<4;i++)
                if (V[i].joinable()) V[i].join();
            if (voteCnt > 5 / 2) {std::cout<<"leader:"<<Port<<"\n";return 1;}
            return 0;
        }
        
        bool Alive()
        {
            std::vector<boost::thread> V;
            for (int i=0;i<5;i++)
            {
                if (50051+i==Port) continue;
               // boost::function0<void> f = boost::bind(&Service::sendAlive, this, i);
                boost::thread th(boost::bind(&Service::sendAlive, this, i));
                V.emplace_back(std::move(th));
            }
            for (int i=0;i<4;i++)
                if (V[i].joinable()) V[i].join();
            return 1;
        }
        
        RPCReply append(const AppendEntiresRPC &message)  
        {
            //std::cout<<Port<<" recieve heartbeat"<<"\n"; 
            Control.interrupt();
        }
        
        RPCReply vote(const RequestVoteRPC &message)
        {
            RPCReply reply(1,currentTerm);
            if (currentTerm > message.term) reply.ans = 0;
            if (votedFor == "") votedFor = message.candidateid;
            return reply;
        }
        
        void Start(uint16_t port) 
        {
            //printf("%d\n",port);
            service.initappend(std::bind(&Service::append, this, std::placeholders::_1));
            service.initvote(std::bind(&Service::vote, this, std::placeholders::_1));
            Control.initElection(std::bind(&Service::Election, this));  
            Control.initAlive(std::bind(&Service::Alive, this));
            std::string server_address("0.0.0.0:" + std::to_string(port));
            std::cout << "Server is listenning in 0.0.0.0:" + std::to_string(port) << "\n";
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            serv=builder.BuildAndStart();
            Port = port;
            runningThread = std::thread([this] { serv -> Wait(); });
            Control.start();
        }
        
        void Shutdown() 
        {
            if (serv) 
                serv -> Shutdown();
            Control.stop();
            runningThread.join();
        }
          
  
    private:
        ServiceImpl service;
        std::unique_ptr<Server> serv;
        std::thread runningThread;
        int currentTerm;
        std::string votedFor = "";
        uint16_t Port;
        heartbeat Control;
        std::atomic<int> voteCnt;
};

#endif 
