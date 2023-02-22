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

LoginResponse* Session::login(const std::string &username) {
    LoginResponse *newLogin = new LoginResponse();
    newLogin->set_username(username);
    newLogin->set_message("You have successfully logged in.");
    ++d_userCount;
    return newLogin;
}

void Session::join(ServerReaderWriter<StreamResponse, StreamRequest> *stream, const std::string &username)
{
    StreamResponse responseMessage;

    if (d_allStreams.find(username) != d_allStreams.end())
    // if user is already logged in, we want to log out that existing login, so that the new login remains active
    {
        d_mutex.lock();
        LogoutResponse *forced_logout_response = new LogoutResponse();
        forced_logout_response->set_username(username);
        forced_logout_response->set_message("There is a new login for your username " + username + ", so logging you out.");
        responseMessage.set_allocated_logout_response(forced_logout_response);

        // write a logout message...
        d_allStreams[username]->Write(responseMessage);

        // ...then delete username from the mapping of active sessions (will add back later in this method)
        d_allStreams.erase(username);
        --d_userCount;
        d_mutex.unlock();
    }

    // process the new login
    std::cout << "Successful login for user: " << username << "\n";

    d_mutex.lock();
    // store this account into accounts list (no matter whether or not said account is already there)
    allAccounts.insert(username);

    // mark user as logged in, and "record" this stream in the d_allStreams dictionary
    // this stream recording is so that the server sends message to the correct recipient client,
    // via that client's stream
    d_allStreams[username] = stream;
    
    // create a LoginResponse object to send back to the client
    LoginResponse *newLogin = login(username);
    d_mutex.unlock();

    // if the user was previously logged in, we send them any messages they missed...
    if (logged_out_users.find(username) != logged_out_users.end()) {
        // iterate through MessageList value of logged out users[username],
        // and send a concatenation of all these messages back to the client
        Message* queued_message = new Message();
        queued_message->set_recipient_username(username);
        queued_message->set_sender_username("Server");
        // adding all messages to one collective string (so that it's easier to send to client)
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

    // tell client they have successfully logged in
    responseMessage.set_allocated_login_response(newLogin);
    stream->Write(responseMessage);
}

ListAccountsResponse* Session::listAccounts(StreamRequest message, std::set<std::string> &allAccounts) {
    std::string username = message.listaccounts_request().username();
    std::string wildcard = message.listaccounts_request().wildcard();

    std::string matching_accounts;
    // iterate through all usernames (both active and logged out)
    // and send back to client the usernames for which wildcard is a substring
    for (std::string a : allAccounts)
    {
        std::cout << "Here\n";
        std::cout << a << "\n";
        // if a username contains wildcard, add to string
        if (a.find(wildcard) != std::string::npos) {
            matching_accounts += a;
            matching_accounts += '\n';
        }
    }

    // create a ListAccountResponse object to store the information
    ListAccountsResponse *listAccounts_response = new ListAccountsResponse();
    listAccounts_response->set_username(username);
    listAccounts_response->set_accounts(matching_accounts);
    return listAccounts_response;
}

LogoutResponse* Session::logout(StreamRequest message) {
    std::string username = message.logout_request().username();

    LogoutResponse *newLogout = new LogoutResponse;
    newLogout->set_username(username);
    newLogout->set_message("You have successfully logged out.");

    // erase username from active streams mapping
    // note that allAccounts still has this username
    d_allStreams.erase(username);

    --d_userCount;
    return newLogout;
}

DeleteAccountResponse* Session::deleteAccount(StreamRequest message, std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> &expected_d_allStreams, std::set<std::string> &allAccounts) {
    std::string username = message.deleteaccount_request().username();

    DeleteAccountResponse *newDeleteAccount = new DeleteAccountResponse;

    newDeleteAccount->set_message("You have successfully deleted your account.");
    newDeleteAccount->set_username(username);

    // erase username from active streams mapping
    // AND from the set of allAccounts
    d_allStreams.erase(username);
    allAccounts.erase(username);

    --d_userCount;
    return newDeleteAccount;
}

int Session::userCount()
{
    return d_userCount;
}

Status ChatImpl::ChatStream(ServerContext *context, ServerReaderWriter<StreamResponse, StreamRequest> *stream) {
    // operation that server listens for from any client
    StreamRequest message;

    while (stream->Read(&message))
    {
        // response server sends to the appropriate client
        StreamResponse responseMessage;
        std::string username = "";
        if (message.has_login_request())
        {
            // server logs the new login
            username = message.login_request().username();
            
            std::cout << "Login request received from " << username << "\n";

            d_mutex.lock();
            // handle this new login
            session->join(stream, username);
            d_mutex.unlock();
        }
        else if (message.has_listaccounts_request())
        {
            std::cout << "List accounts request received\n";

            username = message.listaccounts_request().username();

            std::cout << "Listing accounts for user " << username << "\n";

            d_mutex.lock();
            
            // call the member method in the Session class to generate a ListAccountsResponse
            ListAccountsResponse *listAccounts_response = session->listAccounts(message, allAccounts);

            // send accounts list to the client that requested it
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
            std::cout << "Sender: " << username << ". Intended recipient: " << recipient_username;
            
            if (session->d_allStreams.find(recipient_username) != session->d_allStreams.end()) {
                // send message to a currently logged in user
                newMessage->set_recipient_username(recipient_username);
                newMessage->set_message(message_string);
                std::cout << ". Message: " << message_string << "\n";
                responseMessage.set_allocated_message_response(newMessage);
                session->d_allStreams[recipient_username]->Write(responseMessage);
            } else if (session->logged_out_users.find(recipient_username) != session->logged_out_users.end()) {
                // queue message to a logged out user whose account exists in the system
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
                // send the error message back to the client itself
                responseMessage.set_allocated_message_response(newMessage);
                stream->Write(responseMessage);
            }
            d_mutex.unlock();
        }
        else if (message.has_logout_request())
        {
            // user to be logged out
            username = message.logout_request().username();

            d_mutex.lock();
            LogoutResponse *newLogout = session->logout(message);

            responseMessage.set_allocated_logout_response(newLogout);

            // print out how many active users there are
            std::cout << "Logout request received from " << username << ". Now, there are " << session->d_userCount << " users active.\n";

            stream->Write(responseMessage);
            
            // add this user as a key in the logged_out_users mapping
            Message* queued_message = session->logged_out_users[username].add_messages();
            queued_message->set_recipient_username(username);
            queued_message->set_sender_username("Server");
            queued_message->set_message("Messages you missed when you were gone:\n");

            d_mutex.unlock();
        }
        else if (message.has_deleteaccount_request())
        {
            // user to be deleted from the system
            username = message.deleteaccount_request().username();

            d_mutex.lock();
            DeleteAccountResponse *newDeleteAccount = session->deleteAccount(message, d_allStreams, allAccounts);

            responseMessage.set_allocated_deleteaccount_response(newDeleteAccount);

            std::cout << "Delete account request received from " << username << ". Now, there are " << session->d_userCount << " users active.\n";
            stream->Write(responseMessage);
            d_mutex.unlock();
        }
        else
        {
            std::cout << "Unsupported message type.\n";
        }
    }
    return Status::OK;
}

void runServer()
{
	ChatImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << SERVER_ADDRESS << "\n";
	server->Wait();
}

int main(int argc, char **argv)
{
	std::srand(std::time(nullptr)); // use current time as seed for random number generator
    session = new Session();
	runServer();
    delete session;
	return 0;
}
