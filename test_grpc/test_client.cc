#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include "test_proto.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using test::Vrequest;
using test::Vresponse;
using test::Vergil;

class Client 
{
 public:
  Client(std::shared_ptr<Channel> channel) : stub_(Vergil::NewStub(channel)) {}
  std::string Test(const std::string& user) 
  {
    Vrequest request;
    request.set_request(user);
    Vresponse reply;
    ClientContext context;
    Status status = stub_->modify(&context, request, &reply);
    if (status.ok()) {
      return reply.response();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Vergil::Stub> stub_;
};

int main(int argc, char** argv) {
  Client greeter(grpc::CreateChannel(
      "0.0.0.0:50052", grpc::InsecureChannelCredentials()));
  std::string user("Vergil");
  std::string reply = greeter.Test(user);
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
