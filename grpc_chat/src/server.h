#ifndef SERVER_H
#define SERVER_H

#include "chat.grpc.pb.h"
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
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

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
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;


// an object of this class represents a chat session where all repicients participate.
// used to help modularize the ChatImpl chat implementation class below
class Session
{
    // used for safe read/write access to client streams data
	std::mutex d_mutex;

public:
    // session constructor
	Session() :  d_mutex(), d_userCount(0) {
    }
    // getter methods
    std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> get_d_AllStreams() {
        return d_allStreams;
    }

    std::set<std::string> getAllAccounts() {
        return allAccounts;
    }
    
    std::unordered_map<std::string, MessageList> get_logged_out_users() {
        return logged_out_users;
    }

    int get_d_userCount() {
        return d_userCount;
    }
    
    // data structures used to keep track of usernames and information associated with them

    // map of each actively logged in username to the stream through which they send requests and receive responses (to and from the server)
    // only actively logged in users have streams associated with them
    std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> d_allStreams;

    // set of all current account usernames (of accounts both logged in and not logged in, as long as they were not deleted)
    std::set<std::string> allAccounts;

    // map of each logged out user's username to a repeated field of Messages sent to that user while they were gone

    // the messages are stored in a MessageList proto with a repeated Message field,
    // but are passed to the client as a Message, whose string message field is the concatenation of all senders
    // and messages that they received when they were gone
    std::unordered_map<std::string, MessageList> logged_out_users; // username : undelivered messages

    // number of active (logged in) users
    int d_userCount;

    // create the LoginResponse object to then be processed in the Server::join() function
    LoginResponse* login(const std::string &username);

    // handles a new login, either of a new user or an existing user
    // if new user, the first login is equivalent to account creation followed by login
    // if old user, the user is sent any messages that they missed while they were gone
    // if same user (as another logged in user with the same username), that other user is automatically logged out, and this new login stays active
	void join(ServerReaderWriter<StreamResponse, StreamRequest> *stream, const std::string &username);

    // create the ListAccountsResponse object when client asks to list accounts
    ListAccountsResponse* listAccounts(StreamRequest message, std::set<std::string> &allAccounts);

    // create the LogoutResponse object when client asks to log out
    LogoutResponse* logout(StreamRequest message);

    // create the DeleteAccountResponse object when client asks to delete their account
    DeleteAccountResponse* deleteAccount(StreamRequest message, std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> &expected_d_allStreams, std::set<std::string> &allAccounts);

	int userCount();
};

class ChatImpl final : public Chat::Service
{
	// used for safe read/write access to client streams data
	std::mutex d_mutex;

public:
    // method that handles all the server logic for chat: handles receiving of all StreamRequests and sending of all StreamResponses for the four operations:
    // 1. messaging another user
    // 2. account listing
    // 3. account deletion
    // 4. logout
	Status ChatStream(ServerContext *context, ServerReaderWriter<StreamResponse, StreamRequest> *stream) override;
};

void runServer();

#endif
