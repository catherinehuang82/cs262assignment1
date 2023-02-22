#include <string>

#include "chat.grpc.pb.h"
#include "server.h"
#include <string>
#include "server.h"
#include <string>
#include <thread>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <atomic>
#include <assert.h>
#include <thread>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <atomic>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

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
using chat::MessageList;
using chat::StreamRequest;
using chat::StreamResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

int main(int argc, char** argv) {
    Session *session;
    // ClientContext context;
    // std::unique_ptr<Chat::Stub> stub_;
    // // ServerReaderWriter<StreamRequest, StreamResponse> *stream_cat;
    // // ServerReaderWriter<StreamRequest, StreamResponse> *stream_karly;

    // set up
    session->allAccounts.insert("karly");
    session->allAccounts.insert("cat");
    session->d_allStreams["karly"] = nullptr;
    session->d_allStreams["cat"] = nullptr;

    // test accounts listing
    ListAccountsRequest *listAccounts_request = new ListAccountsRequest();
    listAccounts_request->set_username("cat");
    listAccounts_request->set_wildcard("a");

    StreamRequest message_listAccounts;
    message_listAccounts.set_allocated_listaccounts_request(listAccounts_request);

    ListAccountsResponse* listAccounts_response = session->listAccounts(message_listAccounts, session->allAccounts);

    assert(listAccounts_response->username() == "cat");
    assert(listAccounts_response->accounts() == "karly\ncat\n");

    std::cout << "List accounts test passed.\n";

    // test account deletion
    DeleteAccountRequest *deleteAccount_request = new DeleteAccountRequest();
    deleteAccount_request->set_username("cat");

    StreamRequest message_deleteAccount;
    message_deleteAccount.set_allocated_deleteaccount_request(deleteAccount_request);
    DeleteAccountResponse* deleteAccount_response = session->deleteAccount(message_deleteAccount, session->d_allStreams, session->allAccounts);

    assert(deleteAccount_response->username() == "cat");
    assert(deleteAccount_response->message() == "You have successfully deleted your account.");
    assert(session->allAccounts.size() == 1);

    std::cout << "All tests passed!\n";
}
