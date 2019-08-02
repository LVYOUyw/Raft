#include <bits/stdc++.h>
#include <grpcpp/grpcpp.h>
#include "external.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;
using grpc::ClientContext;
using external::PutRequest;
using external::GetRequest;
using external::GetReply;
using external::PutReply;
using external::External;


class Client
{
    public:
        Client(std::shared_ptr<Channel> channel) : stub_(External::NewStub(channel)) {}

        void Put(const std::string& key, const std::string& value)
        {
            PutRequest Req;
            PutReply Rep;
            ClientContext cont;
            Req.set_key(key);
            Req.set_value(value);
            stub_ -> Put(&cont, Req, &Rep);
        }

        void Get(const std::string& key)
        {
            GetRequest Req;
            GetReply Rep;
            ClientContext cont;
            Req.set_key(key);
            stub_ -> Get(&cont, Req, &Rep);
            std::cout<<key<<" "<<Rep.value()<<"\n";
        }

    private:
        std::unique_ptr<External::Stub> stub_;
};

int main()
{
    Client test(grpc::CreateChannel("0.0.0.0:50057", grpc::InsecureChannelCredentials()));
    test.Put("Vergil", "123");
    test.Put("Garbreil","456");
    test.Put("123","344");
    test.Get("Vergil");
    return 0;
}
