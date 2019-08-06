#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <unordered_map>
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


int main(int argc, char **argv) {
  std::string cmd;
  int id=atoi(argv[1]);
  while (std::cin.peek() != EOF) {
    std::string type;
    std::cin >> type;
    if (type == "put") {
      std::string k, v;
      std::cin >> k >> v;
	  for (int i=0;i<5;i++)
	  {
	  	  Client c(grpc::CreateChannel("0.0.0.0:"+std::to_string(i+50056), grpc::InsecureChannelCredentials()));
	  	  bool pd=c.Put(k,v);
	  	  if (pd) break;
	  }
    } else {
      std::string k,v;
      std::cin >> k;
      for (int i=0;i<5;i++)
	  {
	  	  Client c(grpc::CreateChannel("0.0.0.0:"+std::to_string(i+50056), grpc::InsecureChannelCredentials()));
	  	  v=c.Get(k);
	  	  if (v!="QAQ") break;
	  }
      std::cout << k << " " << v << std::endl;
    }
  }
  return 0;
}
