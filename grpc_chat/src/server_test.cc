#include <climits>
#include <iostream>


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "server.h"

#include "absl/types/optional.h"

#include <grpc/grpc.h>
#include <grpc/support/log.h>
#include <grpc/support/time.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/test/default_reactor_test_peer.h>
#include <grpcpp/test/mock_stream.h>

#include <grpcpp/security/server_credentials.h>

#include "src/core/lib/gprpp/crash.h"
// #include "src/proto/grpc/testing/duplicate/echo_duplicate.grpc.pb.h"
// #include "src/proto/grpc/testing/echo.grpc.pb.h"
// #include "src/proto/grpc/testing/echo_mock.grpc.pb.h"
#include "chat.grpc.pb.h"
#include "chat_mock.grpc.pb.h"
#include "test/core/util/port.h"
#include "test/core/util/test_config.h"

using chat::Chat;
using chat::Message;
using chat::ListAccountsRequest;
using chat::DeleteAccountRequest;
using chat::LogoutRequest;
using chat::LoginRequest;
using chat::LoginResponse;
using chat::ListAccountsResponse;
using chat::DeleteAccountResponse;
using chat::LogoutResponse;
using chat::StreamRequest;
using chat::StreamResponse;
using std::vector;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::WithArg;
using ::testing::Eq;

class FakeClient {
 public:
  explicit FakeClient(Chat::StubInterface* stub) : stub_(stub) {}
 
//   void DoEcho() {
//     ServerContext *context;
//     StreamRequest request;
//     StreamResponse response;
//     ServerReaderWriter<StreamResponse, StreamRequest> *stream;
//     LoginRequest *login_request = new LoginRequest();
// 	login_request->set_username("catherine");
//     request.set_allocated_login_request(login_request);
//     Status s = stub_->ChatStream(context, stream);
//     EXPECT_EQ(request.message(), response.message());
//     EXPECT_TRUE(s.ok());
//   }
 
  void DoChatStream() {
    StreamRequest login_stream_request;
    StreamResponse login_stream_response;
    ::grpc::ClientContext *context;

    ServerReaderWriter<StreamResponse, StreamRequest> *stream;
    LoginRequest *login_request = new LoginRequest();
	login_request->set_username("catherine");
    login_stream_request.set_allocated_login_request(login_request);
 
    std::unique_ptr<::grpc::ClientReaderWriterInterface<StreamRequest, StreamResponse>> cwri = stub_->ChatStream(context);
 
    EXPECT_TRUE(stream->Read(&login_stream_request));
    EXPECT_TRUE(stream->Write(login_stream_response));
    EXPECT_EQ(login_stream_response.login_response().username(), login_stream_request.login_request().username());

    // request.set_message(msg  "1");
    // EXPECT_TRUE(stream->Write(request));
    // EXPECT_TRUE(stream->Read(&response));
    // EXPECT_EQ(response.message(), request.message());
 
    // request.set_message(msg  "2");
    // EXPECT_TRUE(stream->Write(request));
    // EXPECT_TRUE(stream->Read(&response));
    // EXPECT_EQ(response.message(), request.message());
 
    // stream->WritesDone();
    // EXPECT_FALSE(stream->Read(&response));
 
    // Status s = stream->Finish();
    // EXPECT_TRUE(s.ok());
  }
 
  void ResetStub(Chat::StubInterface* stub) { stub_ = stub; }
 
 private:
  Chat::StubInterface* stub_;
};

class MockTest : public ::testing::Test {
 protected:
  MockTest() {}

  void SetUp() override {
    int port = grpc_pick_unused_port_or_die();
    server_address_ << "localhost:" << port;
    // Setup server
    ServerBuilder builder;
    builder.AddListeningPort(server_address_.str(),
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();
  }

  void TearDown() override { server_->Shutdown(); }

  void ResetStub() {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        server_address_.str(), grpc::InsecureChannelCredentials());
    stub_ = chat::Chat::NewStub(channel);
  }

  std::unique_ptr<chat::Chat::Stub> stub_;
  std::unique_ptr<Server> server_;
  std::ostringstream server_address_;
  ChatImpl service_;
};

// Do one real rpc and one mocked one
TEST_F(MockTest, SimpleRpc) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoChatStream();
  chat::MockChatStub stub;
//   EchoResponse resp;
//   resp.set_message("hello world");
  EXPECT_CALL(stub, ChatStreamRaw(_))
      .Times(AtLeast(1));
  client.ResetStub(&stub);
}

int main(int argc, char** argv) {
  grpc::testing::TestEnvironment env(&argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
