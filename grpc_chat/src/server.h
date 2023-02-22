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

// const std::string SERVER_ADDRESS = "0.0.0.0:50051";

// A object of this class represents a chat session.
// in our implementation, there is only one chat session where all repicients participate
class Session
{
    // perhaps? maybe we need one chat stream per username though
	// std::shared_ptr<std::string, ServerReaderWriter<StreamResponse, StreamRequest>> chat_stream;
	std::mutex d_mutex;
	// std::string d_user;

public:
    // session constructor
    // we no longer need a session ID because we assume only one session
	Session() :  d_mutex(), d_userCount(0) {
    }

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
    
    std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> d_allStreams;
    std::set<std::string> allAccounts;
    std::unordered_map<std::string, MessageList> logged_out_users; // username : undelivered messages
    int d_userCount; // number of active (logged in) users

    void broadcast(StreamResponse message);

    void send_message(std::string recipient_username, StreamResponse message);

	void join(ServerReaderWriter<StreamResponse, StreamRequest> *stream, const std::string &username);

	void leave(const std::string &username);

	int userCount();
};

class ChatImpl final : public Chat::Service
{
	// std::unordered_map<std::string, std::unique_ptr<Session>> d_sessions;
	std::mutex d_mutex;

public:
	Status ChatStream(ServerContext *context, ServerReaderWriter<StreamResponse, StreamRequest> *stream) override;
};

void runServer();

#endif
