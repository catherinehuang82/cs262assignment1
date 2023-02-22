#include "chat.grpc.pb.h"
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

const std::string SERVER_ADDRESS = "0.0.0.0:50051";
Session *session;

void Session::broadcast(StreamResponse message)
{
    for (auto each : d_allStreams)
    {
        (each.second)->Write(message);
    }
}

void Session::join(ServerReaderWriter<StreamResponse, StreamRequest> *stream, const std::string &username)
{
    // the isNewSession argument is unused!
    StreamResponse responseMessage;

    if (d_allStreams.find(username) != d_allStreams.end())
    // if user is already logged in, we want to log out that existing login (to preserve the new one)
    {
        LogoutResponse *forced_logout_response = new LogoutResponse();
        forced_logout_response->set_username(username);
        forced_logout_response->set_message("There is a new login for your username " + username + ", so logging you out.");
        responseMessage.set_allocated_logout_response(forced_logout_response);
        d_allStreams[username]->Write(responseMessage);
        --d_userCount;
        d_allStreams.erase(username);
    }

    // process the new login
    std::cout << "Successful login for user: " << username << "\n";
    // std::lock_guard<std::mutex> lg(d_mutex);
    LoginResponse *newLogin = new LoginResponse();
    newLogin->set_username(username);

    // store this account into accounts list (no matter whether or not said account is already there)
    allAccounts.insert(username);

    // mark user as logged in, and "record" this stream in the allStreams dictionary
    // this stream recording is tso that server sends message to the correct recipient client
    // via that client's stream
    d_allStreams[username] = stream;

    // if the user was previously logged in, we send them any messages they missed...
    if (logged_out_users.find(username) != logged_out_users.end()) {
        // std::string undelivered_messages = logged_out_users[username];
        // TODO: iterate through value of logged out users[username], print each message
        Message* queued_message = new Message();
        queued_message->set_recipient_username(username);
        queued_message->set_sender_username("Server");
        std::string queued_message_str = "";
        for (Message &msg : *logged_out_users[username].mutable_messages()) {
            queued_message_str = queued_message_str + msg.sender_username() + ": " + msg.message() + "\n";
        }
        queued_message->set_message(queued_message_str.substr());
        responseMessage.set_allocated_message_response(queued_message); 
        stream->Write(responseMessage);
        //...and mark this user as no longer logged out
        logged_out_users.erase(username);
    }

    // for debugging purposes
    std::cout << "Accounts list: " << std::endl;
    for (std::string a: allAccounts) {
        std::cout << a << std::endl;
    }

    newLogin->set_message("You have successfully logged in.");
    responseMessage.set_allocated_login_response(newLogin);

    // tell client they have successfully logged in
    stream->Write(responseMessage);
    std::cout << "Debug 2" << std::endl;
    ++d_userCount;
    // broadcast(responseMessage);
    std::cout << "Debug 3" << std::endl;
}

void Session::leave(const std::string &username)
{
    // std::lock_guard<std::mutex> lg(d_mutex);
    --d_userCount;
}

int Session::userCount()
{
    return d_userCount;
}

Status ChatImpl::ChatStream(ServerContext *context, ServerReaderWriter<StreamResponse, StreamRequest> *stream) {
    // std::unique_ptr<Session> newSession(new Session());
    // session = std::move(newSession);
    // session = new Session();
    StreamRequest message;

    while (stream->Read(&message))
    {
        StreamResponse responseMessage;
        // std::string id = "";
        std::string username = "";
        if (message.has_login_request())
        {
            std::cout << "Login request received\n";

            username = message.login_request().username();

            d_mutex.lock();

            std::cout << "Debug 6\n";
            session->join(stream, username);
            std::cout << "Debug 7\n";
            d_mutex.unlock();
        }
        else if (message.has_listaccounts_request())
        {
            std::cout << "List accounts request received\n";

            // id = message.joinsession_request().session_id();
            username = message.listaccounts_request().username();
            std::string wildcard = message.listaccounts_request().wildcard();

            std::string matching_accounts;
            // iterate through keys (usernames)
            for (std::string a : session->allAccounts)
            {
                // if a username contains wildcard, add to string
                if (a.find(wildcard) != std::string::npos) {
                    matching_accounts += a;
                    matching_accounts += '\n';
                }
            }
            // accounts_list_str.pop_back();
            printf("listing accounts...\n");

            d_mutex.lock();
            // send accounts list to client that requested it
            ListAccountsResponse *listAccounts_response = new ListAccountsResponse();
            listAccounts_response->set_username(username);
            listAccounts_response->set_accounts(matching_accounts);

            responseMessage.set_allocated_listaccounts_response(listAccounts_response);
            stream->Write(responseMessage);
            d_mutex.unlock();
        }
        else if (message.has_message_request())
        {
            std::cout << "New message received. ";
            username = message.message_request().sender_username();
            std::string recipient_username = message.message_request().recipient_username();

            std::string message_string = message.message_request().message();

            d_mutex.lock();
            Message *newMessage = new Message();
            newMessage->set_sender_username(username);
            std::cout << "Sender: " << username;
            std::cout << ". Intended recipient: " << recipient_username;
            
            if (session->d_allStreams.find(recipient_username) != session->d_allStreams.end()) {
                // sending message to a currently logged in user
                newMessage->set_recipient_username(recipient_username);
                newMessage->set_message(message_string);
                std::cout << ". Message: " << message_string << std::endl;
                responseMessage.set_allocated_message_response(newMessage);
                session->d_allStreams[recipient_username]->Write(responseMessage);
            } else if (session->logged_out_users.find(recipient_username) != session->logged_out_users.end()) {
                // sending message to a logged out user whose account exists in the system

                // add to the MessageList's repeated proto field
                Message* queued_message = session->logged_out_users[recipient_username].add_messages();
                queued_message->set_recipient_username(recipient_username);
                queued_message->set_sender_username(username);
                queued_message->set_message(message_string);
            }
            else {
                // throw an error if sender inputs a recipient username that does not exist
                newMessage->set_sender_username("Server");
                newMessage->set_message("The recipient you specified does not exist. Please try again.");
                // end the error message back to the client itself
                responseMessage.set_allocated_message_response(newMessage);
                stream->Write(responseMessage);
            }
            d_mutex.unlock();
            std::cout << "Debug 1" << std::endl;
            std::cout << "Debug 4" << std::endl;
        }
        else if (message.has_logout_request())
        {
            // user to be logged out
            username = message.logout_request().username();

            d_mutex.lock();
            LogoutResponse *newLogout = new LogoutResponse;
            newLogout->set_username(username);
            newLogout->set_message("You have successfully logged out.");
            responseMessage.set_allocated_logout_response(newLogout);

            session->d_allStreams.erase(username);
            session->leave(username);

            std::cout << "Logout request received from " << username << ". Now, there are " << session->d_userCount << " users active." << std::endl;
            // session->broadcast(responseMessage);

            // WRITE LAST??
            // https://grpc.github.io/grpc/cpp/classgrpc_1_1_write_options.html
            stream->Write(responseMessage);
            Message* queued_message = session->logged_out_users[username].add_messages();
            queued_message->set_recipient_username(username);
            queued_message->set_sender_username("Server");
            queued_message->set_message("Messages you missed when you were gone:\n");

            d_mutex.unlock();

            // for debugging purposes
            std::cout << "Accounts list: " << std::endl;
            for (std::string a: session->allAccounts) {
                std::cout << a << "\n";
            }
        }
        else if (message.has_deleteaccount_request())
        {
            // user to be logged out
            username = message.deleteaccount_request().username();

            d_mutex.lock();
            DeleteAccountResponse *newDeleteAccount = new DeleteAccountResponse;
            newDeleteAccount->set_username(username);
            newDeleteAccount->set_message("You have successfully deleted your account.");
            responseMessage.set_allocated_deleteaccount_response(newDeleteAccount);

            session->d_allStreams.erase(username);
            session->allAccounts.erase(username);

            session->leave(username);

            std::cout << "Delete account request received from " << username << ". Now, there are " << session->d_userCount << " users active." << std::endl;
            // session->broadcast(responseMessage);

            // WRITE LAST???
            // https://grpc.github.io/grpc/cpp/classgrpc_1_1_write_options.html
            stream->Write(responseMessage);
            d_mutex.unlock();

            // for debugging purposes
            std::cout << "Accounts list: " << std::endl;
            for (std::string a: session->allAccounts) {
                std::cout << a << "\n";
            }
        }
        else
        {
            std::cout << "Unsupported message type\n";
        }
    }
    std::cout << "Debug 5\n";
    return Status::OK;
}

void runServer()
{
	ChatImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << SERVER_ADDRESS << std::endl;
	server->Wait();
}

int main(int argc, char **argv)
{
	std::srand(std::time(nullptr)); // Use current time as seed for random number generator.
    session = new Session();
	runServer();
    delete session;
	return 0;
}
