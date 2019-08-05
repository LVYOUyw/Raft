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
            std::cout<<Rep.status()<<"\n";
        }

        std::string Get(const std::string& key)
        {
            GetRequest Req;
            GetReply Rep;
            ClientContext cont;
            Req.set_key(key);
            stub_ -> Get(&cont, Req, &Rep);
            //std::cout<<key<<" "<<Rep.value()<<"\n";
            return Rep.value();
        }

    private:
        std::unique_ptr<External::Stub> stub_;
};

void test(std::size_t n) {
    Client c(grpc::CreateChannel("0.0.0.0:"+std::to_string(n+50056), grpc::InsecureChannelCredentials()));

    std::mt19937 eng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> sleepTimeDist(0, 100);
    auto randSleep = [&eng, &sleepTimeDist] {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeDist(eng)));
    };

    for (int i = 0; i < 30; ++i) {
        auto str = std::to_string(i);
        c.Put(str, str);
        randSleep();
        std::cout << "Put " << i << std::endl;
    }


    for (int i = 0; i < 30; ++i) {
        auto str = std::to_string(i);
        auto res = c.Get(str);
        if (res != str)
        {
            std::cout<<"****"<<res<<" "<<str<<"******"<<"\n";
            return;
        }
        std::cout << "Get: " << i << std::endl;
        randSleep();
    }
}

int main()
{
    const std::size_t NClient = 3;
    std::vector<std::thread> ts;
    ts.reserve(NClient);
    for (std::size_t i = 0; i < NClient; ++i) {
        ts.emplace_back(std::bind(test, i));
    }
    for (auto & t : ts)
        t.join();


    return 0;
}
