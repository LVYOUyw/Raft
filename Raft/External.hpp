#ifndef PPCA_RAFT_EXTERNAL
#define PPCA_RAFT_EXTERNAL

#include <bits/stdc++.h>
#include <grpcpp/grpcpp.h>
#include "test_proto.grpc.pb.h"
#include "external.grpc.pb.h"
#include <boost/thread/thread.hpp>
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

struct PutRPC
{
    std::string key,value;
    PutRPC(std::string Key = "", std::string Value = "") : key(Key), value(Value) {}
};

class ExternalImpl final : public External::Service
{
    public:
        template <class Func>
        void initput(Func &&f)
        {
            Putt = std::forward<Func>(f);
        }

        template <class Func>
        void initget(Func &&f)
        {
            Gett = std::forward<Func>(f);
        }

        template <class Func>
        void inittell(Func &&f)
        {
            Tellt = std::forward<Func>(f);
        }

        Status Put(ServerContext* context, const PutRequest* request,
                   PutReply* reply)     override
        {
            reply -> set_status(Putt(PutRPC(request -> key(), request -> value())));
            return Status::OK;
        }

        Status Get(ServerContext* context, const GetRequest* request,
                   GetReply* reply)     override
        {
            std::string ss = Gett(request -> key());
            reply -> set_value(ss);
            reply -> set_status(1);
            return Status::OK;
        }

        Status TellLeader(ServerContext* context, const GetRequest* request,
                          GetReply* reply)     override
        {

            Tellt(request -> key());
            return Status::OK;
        }


    private:
        std::function<bool(const PutRPC &)> Putt;
        std::function<std::string(const std::string &)> Gett;
        std::function<void(const std::string &)> Tellt;
};

class ExternalService
{
    public:

        void Start(uint16_t port)
        {
            service.initput(std::bind(&ExternalService::Put, this, std::placeholders::_1));
            service.initget(std::bind(&ExternalService::Get, this, std::placeholders::_1));
            service.inittell(std::bind(&ExternalService::Tell, this, std::placeholders::_1));
            std::string server_address("0.0.0.0:" + std::to_string(port));
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            serv=builder.BuildAndStart();
            Port = port;
            runningThread = std::thread([this] { serv -> Wait(); });
            std::cout << "ExternalServer is listenning in 0.0.0.0:" + std::to_string(port) << "\n";
            runningThread.join();
        }

        bool Put(const PutRPC &message)
        {
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + leader,
            grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            Entry Req;
            Reply Rep;
            ClientContext cont;
            Req.set_key(message.key);
            Req.set_args(message.value);
            auto status = tmp -> LeaderAppend(&cont, Req, &Rep);
            return status.ok()?1:0;
        }

        std::string Get(const std::string &message)
        {
            std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:" + leader,
            grpc::InsecureChannelCredentials());
            std::unique_ptr<Vergil::Stub> tmp = Vergil::NewStub(channel);
            GetV Req;
            GetV Rep;
            ClientContext cont;
            Req.set_key(message);
            tmp -> GetValue(&cont, Req, &Rep);
            return Rep.key();
        }

        void Tell(const std::string &message)
        {
            std::cout<<std::to_string(Port) + ": Receive leader: " + message << "\n";
            leader = message;
        }
    public:
        std::string leader;

    private:
        ExternalImpl service;
        std::unique_ptr<Server> serv;
        std::thread runningThread;
        uint16_t Port;
        //std::string leader;
};
#endif
