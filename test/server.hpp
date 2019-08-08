#ifndef PPCA_RAFT_SERVER
#define PPCA_RAFT_SERVER

#include <bits/stdc++.h>
#include <unistd.h>
#include <grpcpp/grpcpp.h>
#include "test_proto.grpc.pb.h"
#include "external.grpc.pb.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>
#include "heartbeat.hpp"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using test::AppendEntriesMessage;
using test::RequestVoteMessage;
using test::Entry;
using test::Reply;
using test::GetV;
using test::Vergil;
using external::PutRequest;
using external::GetRequest;
using external::GetReply;
using external::PutReply;
using external::External;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

struct RPCReply
{
    bool ans;
    int term;
    int logsiz;

    RPCReply(bool Ans = 0, int Term = 0, int Logsiz = 0) : ans(Ans), term(Term), logsiz(Logsiz){}
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
    int logsiz;
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

template <class Func>
void initgetv(Func &&f)
{
    getv = std::forward<Func>(f);
}

Status GetValue(ServerContext* context, const GetV* request,
                GetV* reply)     override
{
    std::string ans = getv(request -> key());
    reply -> set_key(ans);
    return Status::OK;
}

Status LeaderAppend(ServerContext* context, const Entry* request,
                    Reply* reply)     override
{
    client(EntryRPC(request -> key(), request -> args(), 0));
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
    tmp.logsiz = request -> logsiz();
    for (int i=0;i<siz;i++)
    {
        Entry e = request -> entries(i);
        tmp.Entries.push_back(EntryRPC(e.key(),e.args(),e.term()));
    }
    RPCReply rep=append(tmp);
    reply -> set_term(rep.term);
    reply -> set_ans(rep.ans);
    reply -> set_logsiz(rep.logsiz);
    return Status::OK;
}
private:
    std::function<RPCReply(const AppendEntiresRPC &)> append;
    std::function<RPCReply(const RequestVoteRPC &)> vote;
    std::function<void(const EntryRPC &)> client;
    std::function<std::string(const std::string &)> getv;
};

class Service
{
    public:

        Service()
        {
            currentTerm = 0;
            votedFor = 0;
        }

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
            cont.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(300));
            tmp -> RequestVote(&cont, Req, &Rep);
            if (Rep.ans()) voteCnt++;
            if (Rep.term() > currentTerm)
            {
                voteCnt = -100;
                currentTerm = Rep.term();
            }
        }

        void sendAlive(uint16_t tt)
        {

            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" +
            std::to_string(tt),
            grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            AppendEntriesMessage Req;
            Reply Rep;
            ClientContext cont;
            cont.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(300));
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
        //    printf("nextIndex: %d %d\n",nextIndex[id]-1,id);
            Req.set_prevlogterm(log[nextIndex[id]-1].term);
            Req.set_leadercommit(commitIndex);
            Req.set_logsiz(log.size());
            Entry* entry;

            for (int i=nextIndex[id];i<log.size();i++)
            {
                entry =  Req.add_entries();
                entry -> set_term(log[i].term);
                entry -> set_key(log[i].key);
                entry -> set_args(log[i].value);
            }
            cont.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(300));
            //    printf("send heartbeat to %d\n",id);
            auto status = tmp -> AppendEntries(&cont, Req, &Rep);
            if (Rep.term() > currentTerm || Rep.logsiz() > log.size())
            {
                Control.ToFollower();
                currentTerm = Rep.term();
                return;
            }
            //if (!Rep.ans()) printf("--%d\n--",nextIndex[id]);
            if (status.ok()) nextIndex[id] = Rep.ans() ? log.size() : nextIndex[id] - 1;
            nextIndex[id] = std::max(nextIndex[id], 1);
        /*    while (status.ok() && !Rep.ans())
            {
                Req.set_term(currentTerm);
                Req.set_leaderid(std::to_string(Port));
                Req.set_prevlogindex(nextIndex[id]-1);
                //    printf("nextIndex: %d %d\n",nextIndex[id]-1,id);
                Req.set_prevlogterm(log[nextIndex[id]-1].term);
                Req.set_leadercommit(commitIndex);
                Req.set_logsiz(log.size());
                Entry* entry;
                for (int i=nextIndex[id];i<log.size();i++)
                {
                    entry =  Req.add_entries();
                    entry -> set_term(log[i].term);
                    entry -> set_key(log[i].key);
                    entry -> set_args(log[i].value);
                }
                //cont.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(300));
                status = tmp -> AppendEntries(&cont, Req, &Rep);
                if (status.ok()) nextIndex[id] = Rep.ans() ? log.size() : nextIndex[id] - 1;
            }*/
        }

        bool Election()
        {
            currentTerm++;
            voteCnt = 1;
            std::cout<<"Candidate "<<currentTerm<<" "<<voteCnt<<"\n";
            votedFor = Port;
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
                std::cerr<<"voteCnt:"<<" "<<voteCnt<<" "<<std::to_string(Port)<<" in "<<currentTerm<<"\n";
                if (voteCnt > 5 / 2) {std::cout<<"leader:"<<Port<<" "<<currentTerm<<"\n";LeaderPrepare();return 1;}
                    else votedFor = 0;
            return 0;
        }

        bool Alive()
        {
            commitIndex=log.size()-1;
            while (lastApplied<commitIndex)
            {
                lastApplied++;
                M[log[lastApplied].key]=log[lastApplied].value;
            }
            std::vector<boost::thread> V;
            for (int i=0;i<5;i++)
            {
                if (50051+i==Port) continue;
                // boost::function0<void> f = boost::bind(&Service::sendAlive, this, i);
                boost::thread th(boost::bind(&Service::sendAlive, this, 50051+i));
                V.emplace_back(std::move(th));
            }
            for (int i=0;i<4;i++)
                if (V[i].joinable()) V[i].join();
            if (!Control.leader()) return 0;
            for (int i=0;i<5;i++)
            {
                std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + std::to_string(50051+i+5),
                grpc::InsecureChannelCredentials());
                std::unique_ptr<External::Stub> tmp = External::NewStub(channel);
                GetRequest Req;
                GetReply Rep;
                ClientContext cont;
                cont.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(500));
                Req.set_key(leader);
                tmp -> TellLeader(&cont, Req, &Rep);
            }
            return 1;
        }

        RPCReply append(const AppendEntiresRPC &message)
        {
            RPCReply reply(0,currentTerm,log.size());
            if (currentTerm > message.term || log.size() > message.logsiz) return reply;
            Control.ToFollower();
            Control.interrupt();
            votedFor = 0;
            voteCnt = 0;
            leader = message.leaderid;
            int prevLogIndex = message.prevLogIndex;
            int prevLogTerm = message.prevLogTerm;
            if (currentTerm < message.term) currentTerm = message.term;
            //    std::cout << ("HeartBeat Port: " + std::to_string(Port)) << " " << prevLogIndex << " " <<log.size() << "\n";
            if (log.size()<=message.prevLogIndex) return reply;
            //std::cout << ("HeartBeat Port" + std::to_string(Port)) << " " << prevLogIndex <<"\n";
            if (log[prevLogIndex].term!=prevLogTerm)
            {
                while (log.size()>prevLogIndex) log.pop_back();
                return reply;
            }
            reply.ans=1;
            int siz=message.Entries.size();
            //    printf("%d\n",siz);
            for (int i=0;i<siz;i++) log.push_back(message.Entries[i]);
            if (message.leaderCommit>commitIndex)
            {
                commitIndex=message.leaderCommit;
            //    std::cerr<<Port<<": "<<commitIndex<<" "<<lastApplied<<"\n";
            }
            while (commitIndex>lastApplied&&lastApplied < log.size()-1)
            {
                lastApplied++;
                M[log[lastApplied].key] = log[lastApplied].value;
                //std::cout<<std::to_string(Port)<<" "<<lastApplied<<"\n";
            }
            return reply;
        }

        RPCReply vote(const RequestVoteRPC &message)
        {
            RPCReply reply(1,currentTerm);
            if (votedFor == 0) votedFor = atoi(message.candidateid.c_str());else reply.ans = 0;
            if (currentTerm > message.term)
                reply.ans = 0;
            if (currentTerm < message.term)
            {
                currentTerm = message.term;
                Control.ToFollower();
                votedFor = 
            }
            std::cerr << Port << " vote for " << votedFor << " in " << currentTerm << " " <<message.term<<"\n";

            return reply;
        }

        void Leader(const EntryRPC &message)
        {
            boost::mutex mutex;
            boost::lock_guard<boost::mutex> lock(mutex);
            EntryRPC tmp=message;
            tmp.term=currentTerm;
            log.push_back(tmp);
            Alive();
            //std::cerr << "Leader:" << std::to_string(Port) << " Put " << message.key << " " << message.value << std::endl;
            //std::cerr << "Leader commit id " << commitIndex << " " << log.size() << "\n";
            //boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            while (lastApplied != log.size() - 1) boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }

        std::string GetV(const std::string & s)
        {
            std::cerr<<"Port: "<<std::to_string(Port) <<"\n";
            while (commitIndex != log.size()-1) boost::this_thread::sleep_for(boost::chrono::milliseconds(300));
            return M[s];
        }

        void LeaderPrepare()
        {
            Control.ToLeader();
            votedFor = 0;
            nextIndex.clear();
            matchIndex.clear();
            int siz=log.size();
            leader = std::to_string(Port);
            for (int i=0;i<4;i++) nextIndex.push_back(siz);
            Alive();
            if (Control.leader())
                std::cerr << "Leader " << std::to_string(Port) << " "<<commitIndex << " " << log.size() << "\n";
        }

        void Start(uint16_t port)
        {
            service.initappend(std::bind(&Service::append, this, std::placeholders::_1));
            service.initvote(std::bind(&Service::vote, this, std::placeholders::_1));
            service.initclient(std::bind(&Service::Leader, this, std::placeholders::_1));
            service.initgetv(std::bind(&Service::GetV, this, std::placeholders::_1));
            Control.initElection(std::bind(&Service::Election, this));
            Control.initAlive(std::bind(&Service::Alive, this));
            std::string server_address("0.0.0.0:" + std::to_string(port));
            //        std::cout << "Server is listenning in 0.0.0.0:" + std::to_string(port) << "\n";
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            serv=builder.BuildAndStart();
            Port = port;
            runningThread = std::thread([this] { serv -> Wait(); });
            log.push_back((EntryRPC("","",0)));
            Control.start();
        //    runningThread.join();
        }

        void Shutdown()
        {
            if (serv) serv -> Shutdown();
            Control.stop();
            runningThread.join();
        }

    public:
        uint16_t Port;

    private:
        ServiceImpl service;
        std::unique_ptr<Server> serv;
        std::thread runningThread;;
        //uint16_t Port;
        std::string leader;
        heartbeat Control;
        std::atomic<int> voteCnt,currentTerm,votedFor;
        std::vector<EntryRPC> log;
        int commitIndex = 0, lastApplied = 0;
        std::vector<int> nextIndex, matchIndex;
        std::map<std::string, std::string> M;
};

#endif
