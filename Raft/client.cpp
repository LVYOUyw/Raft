#include <bits/stdc++.h>
#include <unistd.h>
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

        bool Put(const std::string& key, const std::string& value)
        {
            PutRequest Req;
            PutReply Rep;
            ClientContext cont;
            Req.set_key(key);
            Req.set_value(value);
            auto status = stub_ -> Put(&cont, Req, &Rep);
            return status.ok();
        }

        std::string Get(const std::string& key)
        {
            GetRequest Req;
            GetReply Rep;
            ClientContext cont;
            Req.set_key(key);
            auto status = stub_ -> Get(&cont, Req, &Rep);
            //std::cout<<key<<" "<<Rep.value()<<"\n";
            return status.ok()?Rep.value():"QAQ";
        }

    private:
        std::unique_ptr<External::Stub> stub_;
};

void test(int a) {
    Client c(grpc::CreateChannel("0.0.0.0:"+std::to_string(a), grpc::InsecureChannelCredentials()));

    for (int i = 0; i < 30; ++i) {
        auto str = std::to_string(i);
        c.Put(str, str);
        std::cout << "Put " << i << std::endl;
    }


    for (int i = 0; i < 30; ++i) {
        auto str = std::to_string(i);
        auto res = c.Get(str);
        assert(str == res);
        std::cout << "Get: " << i << std::endl;
    }
}

int main()
{
    int a;
    scanf("%d",&a);
    test(a);
    return 0;
}
