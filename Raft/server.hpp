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
using test::Entry;
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
struct EntryRPC
{
    std::string key,value;
    int term;

    EntryRPC(std::string Key = "", std::string Value = "", int Term = 0) :
                                                                        key(Key),value(Value),term(Term) {}
};
struct AppendEntiresRPC
{
    int term;
    std::string leaderid;
    std::vector<EntryRPC> Entries;
    int prevLogIndex;
    int prevLogTerm;
    int leaderCommit;
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

        template <class Func>
        void initclient(Func &&f)
        {
            client = std::forward<Func>(f);
        }

        Status LeaderAppend(ServerContext* context, const Entry* request,
                           Reply* reply)     override
        {
            client(EntryRPC(request -> key(), request -> value(), 0));
            return Status::OK;
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
            int siz = request -> entries_size();
            AppendEntiresRPC tmp;
            tmp.term = request -> term();
            tmp.leaderid = request -> leaderid();
            tmp.prevLogIndex = request -> prevlogindex();
            tmp.prevLogTerm = request -> prevlogterm();
            tmp.leaderCommit = request -> leadercommit();
            for (int i=0;i<siz;i++)
            {
                Entry e = request -> entries(i);
                tmp.Entries.push_back(EntryRPC(e.key(),e.args(),e.term()));
            }
            RPCReply rep=append(tmp);
            reply -> set_term(rep.term);
            reply -> set_ans(rep.ans);
            return Status::OK;
        }
    private:
         std::function<RPCReply(const AppendEntiresRPC &)> append;
         std::function<RPCReply(const RequestVoteRPC &)> vote;
         std::function<void(EntryRPC &)> client;
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
            ClientContext cont;
            int id=0;
            for (int i=0;i<5;i++)
            {
                if (50051+i==Port) continue;
                if (50051+i==tt) break;
                id++;
            }
            Req.set_term(currentTerm);
            Req.set_leaderid(std::to_string(Port));
            Req.set_prevlogindex(nextIndex[id]-1);
            Req.set_prevlogterm(log[nextIndex[id]-1].term);
            Req.set_leadercommit(commitIndex);
            Entry* entry;
            for (int i=nextIndex[id]-1;i<log.size();i++)
            {
                entry = Req.entries.add_entry();
                entry -> set_term(log[i].term);
                entry -> set_key(log[i].key);
                entry -> set_args(log[i].value);
            }
            cont.set_deadline(boost::chrono::system_clock::now() + std::chrono::milliseconds(500));
            auto status = tmp -> AppendEnties(&cont, Req, &Rep);
            if (status.ok() && Rep.ans) nextIndex[id]=log.size();
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
            if (voteCnt > 5 / 2) {std::cout<<"leader:"<<Port<<"\n";LeaderPrepare();return 1;}
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
            int cnt=0;
            for (int i=0;i<4;i++)
                if (nextIndex[i]==log.size()) cnt++;
            if (cnt>=2)
                commitIndex=log.size()-1;
            while (lastApplied<commitIndex)
            {
                lastApplied++;
                M[log[lastApplied].key]=log[lastApplied].value;
            }
            return 1;
        }

        void Leader(EntryRPC &message)
        {
            message.term=currentTerm;
            log.push_back(message);
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        }

        RPCReply append(const AppendEntiresRPC &message)
        {
            Control.interrupt();
            leaderid = message.leaderId;
            if (currentTerm < message.term) currentTerm = message.term;
            RPCReply reply(0,currentTerm);
            if (log.size()<=message.prevLogIndex) return reply;
            if (log[prevLogIndex].term!=prevLogTerm)
            {
                while (log.size()>prevLogIndex) log.pop_back();
                return reply;
            }
            reply.ans=1;
            int siz=message.Entries.size();
            for (int i=0;i<siz;i++) log.push_back(message.Entries[i]);
            if (message.leaderCommit>commitIndex)
                commitIndex=min(leaderCommit,log.size()-1);
            while (commitIndex>lastApplied)
            {
                lastApplied++;
                M[log[lastApplied].key] = log[lastApplied].value;
            }
            return reply;
        }

        RPCReply vote(const RequestVoteRPC &message)
        {
            RPCReply reply(1,currentTerm);
            if (currentTerm > message.term) reply.ans = 0;
            if (votedFor == "") votedFor = message.candidateid;
            return reply;
        }

        void LeaderPrepare()
        {
            nextIndex.clear();
            matchIndex.clear();
            int siz=log.size();
            leader = Port;
            for (int i=0;i<4;i++) nextIndex.push(siz);
        }

        void Start(uint16_t port)
        {
            service.initappend(std::bind(&Service::append, this, std::placeholders::_1));
            service.initvote(std::bind(&Service::vote, this, std::placeholders::_1));
            service.initclient(std::bind(&Service::vote, this,))
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
            log.push_back((EntryRPC("","",0)));
            Control.start();
        }0

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
        int currentTerm = 0;
        std::string votedFor = "";
        uint16_t Port,leader;
        heartbeat Control;
        std::atomic<int> voteCnt;
        std::atomic<int>
        std::vector<EntryRPC> log;
        int commitIndex = 0, lastApplied = 0;
        std::vector<int> nextIndex, matchIndex;
        std::map<std::string, std::string> M;
};

#endif
